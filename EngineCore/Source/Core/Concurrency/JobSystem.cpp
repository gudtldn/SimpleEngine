// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Core/Concurrency/JobSystem.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Debug.h"

#include <algorithm>
#include <chrono>
#include <limits>


namespace se
{
namespace
{
using namespace std::literals;

/** 유휴 상태에서 대기 전 스핀 반복 횟수 */
constexpr usize IDLE_SPIN_COUNT = 32;

/** condition_variable 대기 타임아웃 */
constexpr std::chrono::milliseconds IDLE_WAIT_TIMEOUT = 1ms;

/** TLS에 워커 인덱스를 저장하는 변수. max()면 비워커 스레드 */
thread_local usize CurrentWorkerIndex = std::numeric_limits<usize>::max();
} // namespace

JobSystem* JobSystem::instance = nullptr;


JobSystem::JobSystem(usize in_worker_count)
{
    SE_ASSERT(!JobSystem::instance, "JobSystem instance already exists!");
    JobSystem::instance = this;

    if (in_worker_count == 0)
    {
        const uint32 hw_threads = std::thread::hardware_concurrency();
        worker_count = std::max<usize>(1, hw_threads - 1);
    }
    else
    {
        worker_count = in_worker_count;
    }

    ConsoleLog(ELogLevel::Info, "JobSystem: Initializing with {} worker threads", worker_count);

    // 워커 데이터 배열 할당
    worker_states = std::make_unique<WorkerState[]>(worker_count);

    // 워커 스레드 생성
    worker_threads.Reserve(worker_count);
    for (usize idx = 0; idx < worker_count; ++idx)
    {
        worker_threads.Emplace([this, idx](const std::stop_token& st)
        {
            WorkerLoop(st, idx);
        });
    }
}

JobSystem::~JobSystem()
{
    SE_ASSERT(JobSystem::instance == this, "JobSystem instance mismatch!");

    // jthread 소멸자에서 request_stop() + join()을 호출
    worker_threads.Clear();

    // Global Inbox 모두 해제
    const JobPayload* inbox_head = global_inbox.exchange(nullptr, std::memory_order_acquire);
    while (inbox_head)
    {
        const JobPayload* next = inbox_head->next_pending;
        delete inbox_head;
        inbox_head = next;
    }

    // 워커 Deque 모두 해제
    for (usize w = 0; w < worker_count; ++w)
    {
        for (usize p = 0; p < NUM_JOB_PRIORITIES; ++p)
        {
            while (Optional<JobPayload*> opt = worker_states[w].deques[p].Pop())
            {
                delete *opt;
            }
        }
    }

    JobSystem::instance = nullptr;
    ConsoleLog(ELogLevel::Info, "JobSystem: Destroyed");
}

JobSystem& JobSystem::Get()
{
    SE_ASSERT(JobSystem::instance, "JobSystem instance is not initialized!");
    return *JobSystem::instance;
}

bool JobSystem::IsInitialized()
{
    return JobSystem::instance != nullptr;
}

void JobSystem::DispatchToMain(UniqueFunction<void()>&& work_func)
{
    main_queue.Push(std::move(work_func));
}

usize JobSystem::ExecuteMainThreadJobs()
{
    return main_queue.Drain();
}

bool JobSystem::TryExecuteOneJob()
{
    JobPayload* payload = nullptr;

    // 워커 스레드라면 자신의 Deque에서 먼저 시도
    if (CurrentWorkerIndex < worker_count)
    {
        payload = TryPopLocal(CurrentWorkerIndex);
    }

    // Global Inbox에서 시도
    if (!payload)
    {
        payload = TryPopGlobal();
    }

    // 다른 워커에서 Steal을 시도
    if (!payload)
    {
        payload = TryStealFromOthers(CurrentWorkerIndex);
    }

    if (payload)
    {
        ExecutePayload(payload);
        return true;
    }

    return false;
}

usize JobSystem::GetWorkerCount() const
{
    return worker_count;
}

void JobSystem::EnqueuePayload(JobPayload* payload)
{
    if (CurrentWorkerIndex < worker_count)
    {
        // 워커 스레드라면 자신의 Deque에 직접 Push (Owner만 Push 가능)
        const usize priority = static_cast<usize>(payload->priority);
        worker_states[CurrentWorkerIndex].deques[priority].Push(payload);
    }
    else
    {
        // 비워커 스레드면 Global Inbox에 Lock-Free Push (Treiber Stack)
        JobPayload* old_head = global_inbox.load(std::memory_order_relaxed);
        do
        {
            // 아직 이 시점에서는 payload가 Global Inbox에 등록되지 않았기 때문에,
            // Lock이나 원자적으로 접근할 필요없이 값 수정
            payload->next_pending = old_head;
        }
        while (!global_inbox.compare_exchange_weak(
            old_head, payload,
            std::memory_order_release, std::memory_order_relaxed
        ));

        /* global_inbox.compare_exchange_weak(
         *     old_head, payload,
         *     std::memory_order_release, std::memory_order_relaxed
         * ); 를 풀어쓰면
         *
         * const T actual_val = global_inbox.load(...); // 내부적으로 success failure의 order중 더 강한걸로 설정
         * if (actual_val == old_head)
         * {
         *     global_inbox.store(payload, std::memory_order_release);
         *     return true;
         * }
         * else
         * {
         *     old_head = actual_val;
         *     return false;
         * }
         * 라고 할 수 있다. (실제로는 위 내용이 원자적으로 실행됨)
         *
         * 즉, global_inbox == old_head면 현재 global_inbox가 최신 값이니 payload를 적용해라 라는 뜻이고,
         * global_inbox != old_head면 global_inbox가 다른 스레드에서 수정되었으니 다시 읽어라 라는 뜻.
         */
    }

    // 대기 중인 워커를 하나 깨운다
    wake_cv.notify_one();
}

void JobSystem::ExecutePayload(JobPayload* payload)
{
    ZoneScopedN("JobSystem::ExecutePayload");

    payload->Invoke();

    // 완료 카운터를 감소 (Waiter 통지 + 의존성 해소 트리거)
    if (payload->completion_counter)
    {
        payload->completion_counter->Decrement();
    }

    delete payload;
}

void JobSystem::WorkerLoop(const std::stop_token& stoken, usize worker_index)
{
    // TLS에 워커 인덱스를 기록
    CurrentWorkerIndex = worker_index;

    // 스레드 이름 설정
    const String thread_name = String::Format("JobWorker {}", worker_index);
    Platform::SetCurrentThreadName(thread_name);

    usize idle_spins = 0;
    while (!stoken.stop_requested())
    {
        // 1. Global Inbox에서 자신의 Deque로 이동 (Owner Push 보장)
        if (JobPayload* inbox_item = TryPopGlobal())
        {
            const usize priority = static_cast<usize>(inbox_item->priority);
            worker_states[worker_index].deques[priority].Push(inbox_item);
        }

        // 2. 자신의 Deque에서 우선순위 순으로 Pop 시도
        JobPayload* payload = TryPopLocal(worker_index);

        // 3. 로컬에 없으면 다른 워커에서 Steal 시도
        if (!payload)
        {
            payload = TryStealFromOthers(worker_index);
        }

        // 3. 작업 발견 시 실행, 아니면 백오프
        if (payload)
        {
            ExecutePayload(payload);
            idle_spins = 0;
        }
        else if (idle_spins < IDLE_SPIN_COUNT)
        {
            // 짧은 스핀 대기 (컨텍스트 스위칭 비용 회피)
            std::this_thread::yield();
            ++idle_spins;
        }
        else
        {
            // 스핀 한도 초과 -> condition_variable로 수면
            std::unique_lock lock{ wake_mutex };
            wake_cv.wait_for(lock, stoken, IDLE_WAIT_TIMEOUT, [] { return false; });
            idle_spins = 0;
        }
    }
}

JobPayload* JobSystem::TryPopLocal(usize worker_index)
{
    // 높은 우선순위부터 순차적으로 확인
    for (usize p = 0; p < NUM_JOB_PRIORITIES; ++p)
    {
        if (const Optional<JobPayload*> result = worker_states[worker_index].deques[p].Pop())
        {
            return *result;
        }
    }
    return nullptr;
}

JobPayload* JobSystem::TryStealFromOthers(usize thief_index)
{
    // 높은 우선순위부터, 각 워커를 순회하며 Steal을 시도
    for (usize p = 0; p < NUM_JOB_PRIORITIES; ++p)
    {
        for (usize w = 0; w < worker_count; ++w)
        {
            const usize target = (thief_index + 1 + w) % worker_count;

            // 자기 자신은 스킵
            if (target == thief_index)
            {
                continue;
            }

            if (const Optional<JobPayload*> result = worker_states[target].deques[p].Steal())
            {
                return *result;
            }
        }
    }
    return nullptr;
}

JobPayload* JobSystem::TryPopGlobal()
{
    // Global Inbox에서 Payload를 하나 꺼낸다
    JobPayload* head = global_inbox.load(std::memory_order_acquire);
    while (head)
    {
        if (global_inbox.compare_exchange_weak(
            head, head->next_pending,
            std::memory_order_acq_rel, std::memory_order_acquire
        ))
        {
            head->next_pending = nullptr;
            return head;
        }
    }
    return nullptr;
}
} // namespace se
