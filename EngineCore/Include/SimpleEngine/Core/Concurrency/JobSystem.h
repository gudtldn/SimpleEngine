#pragma once

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Concurrency/JobHandle.h"
#include "SimpleEngine/Core/Concurrency/JobPayload.h"
#include "SimpleEngine/Core/Concurrency/MpscTaskLinkedQueue.h"
#include "SimpleEngine/Core/Concurrency/WorkStealingDeque.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "tracy/Tracy.hpp"

#include <algorithm>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>


namespace se
{
/**
 * Lambda-Core 기반 Job System (전역 싱글톤)
 *
 * 모든 비동기 작업의 중앙 진입점입니다.
 * Per-Worker Work-Stealing Deque와 우선순위 기반 스케줄링을 제공합니다.
 */
class SE_CORE_API JobSystem
{
    static JobSystem* instance;

public:
    /**
     * JobSystem을 초기화합니다.
     * @param in_worker_count 워커 스레드 수 (0이면 하드웨어 코어 수 기반 자동 결정)
     */
    explicit JobSystem(usize in_worker_count = 0);
    ~JobSystem();

    // 복사 & 이동 금지
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    /** JobSystem 싱글톤 인스턴스를 반환합니다. */
    static JobSystem& Get();

    /** JobSystem 인스턴스가 초기화되어 있는지 확인합니다. */
    [[nodiscard]] static bool IsInitialized();

public:
    /**
     * 완료 추적 없이 작업을 워커 스레드로 전송합니다. (Fire-and-Forget)
     *
     * @tparam Fn void()로 호출 가능한 callable 타입
     * @param work_func 실행할 callable
     * @param priority 실행 우선순위
     */
    template <typename Fn>
        requires std::invocable<Fn>
    void Dispatch(Fn&& work_func, EJobPriority priority = EJobPriority::Normal);

    /**
     * 완료 추적 없이 작업을 메인 스레드 큐로 전송합니다. (모든 스레드에서 호출 가능)
     *
     * @param work_func 메인 스레드에서 실행할 작업
     */
    void DispatchToMain(UniqueFunction<void()>&& work_func);

    /**
     * 작업을 워커 스레드에 제출하고, 완료 상태를 추적할 수 있는 핸들을 반환합니다.
     *
     * @tparam Fn void()로 호출 가능한 callable 타입
     * @param work_func 실행할 callable (람다, 함수 객체 등)
     * @param priority 실행 우선순위
     * @return 작업 완료를 추적하는 핸들
     */
    template <typename Fn>
        requires std::invocable<Fn>
    JobHandle Submit(Fn&& work_func, EJobPriority priority = EJobPriority::Normal);

    /**
     * 선행 작업들이 모두 완료된 후 실행될 의존성 작업을 제출합니다.
     *
     * @tparam Fn void()로 호출 가능한 callable 타입
     * @param work_func 실행할 callable
     * @param dependencies 선행 작업 핸들 목록
     * @param priority 실행 우선순위
     * @return 작업 완료를 추적하는 핸들
     */
    template <typename Fn>
        requires std::invocable<Fn>
    JobHandle Submit(Fn&& work_func, ArrayView<const JobHandle> dependencies, EJobPriority priority = EJobPriority::Normal);

    /**
     * 지정된 범위를 배치 단위로 분할하여 워커 스레드들에 병렬로 제출합니다.
     *
     * @tparam Fn void(usize index)로 호출 가능한 callable 타입
     * @param in_count 총 반복 횟수
     * @param batch_size 배치당 반복 횟수
     * @param work_func 각 인덱스에 대해 호출될 callable
     * @param priority 실행 우선순위
     * @return 모든 배치의 완료를 통합하여 추적하는 단일 핸들
     */
    template <typename Fn>
        requires std::invocable<Fn, usize>
    JobHandle ParallelFor(usize in_count, usize batch_size, Fn work_func, EJobPriority priority = EJobPriority::Normal);

    /**
     * 현재 메인 스레드 큐에 대기 중인 모든 작업을 일괄 실행(Drain)합니다. (메인 스레드 전용)
     *
     * @return 실행된 작업 수
     * @warning 반드시 메인 스레드에서만 호출해야 합니다.
     */
    usize ExecuteMainThreadJobs();

public:
    /**
     * 현재 스레드가 대기(Wait) 상태일 때, CPU 유휴를 방지하기 위해 다른 Job을 훔쳐서 실행합니다.
     * @return 작업을 성공적으로 찾아 실행했다면 true
     */
    bool TryExecuteOneJob();

    /** 워커 스레드 수를 반환합니다. */
    [[nodiscard]] usize GetWorkerCount() const;

private:
    /** 생성된 JobPayload를 현재 스레드 컨텍스트에 맞춰 적절한 큐(Local Deque 또는 Global Inbox)에 삽입합니다. */
    void EnqueuePayload(JobPayload* payload);

    /** JobPayload를 실행하고, 완료 카운터를 감소시킨 뒤 메모리를 해제합니다. */
    static void ExecutePayload(JobPayload* payload);

    /** 워커 스레드의 메인 루프 */
    void WorkerLoop(const std::stop_token& stoken, usize worker_index);

    /** 자신의 Deque에서 우선순위가 높은 순서대로 Pop을 시도합니다. */
    JobPayload* TryPopLocal(usize worker_index);

    /** 다른 워커들의 Deque에서 우선순위가 높은 순서대로 Steal을 시도합니다. */
    JobPayload* TryStealFromOthers(usize thief_index);

    /** 글로벌 인박스(Treiber Stack)에서 최신 Payload를 하나 꺼냅니다. */
    JobPayload* TryPopGlobal();

private:
    /** 워커 스레드별 데이터. False Sharing 방지를 위해 캐시 라인 정렬됩니다. */
    struct alignas(SE_CACHE_LINE) Worker
    {
        /** 우선순위별 Work-Stealing Deque (Critical=0, Normal=1, Low=2) */
        FixedArray<WorkStealingDeque<JobPayload*>, NUM_JOB_PRIORITIES> deques;
    };

    /** 워커 데이터 배열 (Deque 포함) */
    std::unique_ptr<Worker[]> workers;

    /** 워커 스레드 배열 */
    Array<std::jthread> threads;

    /** 메인 스레드 전용 MPSC 큐 */
    MpscTaskLinkedQueue main_queue;

    /** 워커 스레드 수 */
    usize worker_count = 0;

    /**
     * 글로벌 인박스 (Treiber Stack)
     *
     * Chase-Lev Deque의 Push는 Owner 스레드에서만 호출 가능하므로,
     * 비워커 스레드(메인, 외부)에서 제출된 Job은 이 Lock-Free 스택에
     * 먼저 적재되며, 워커가 루프에서 꺼내어 자신의 Deque로 이동시킵니다.
     */
    alignas(SE_CACHE_LINE) std::atomic<JobPayload*> global_inbox = nullptr;

    /** 워커 대기/깨우기용 동기화 프리미티브 */
    TracyLockable(std::mutex, wake_mutex);
    std::condition_variable_any wake_cv;
};

template <typename Fn>
    requires std::invocable<Fn>
void JobSystem::Dispatch(Fn&& work_func, EJobPriority priority)
{
    JobPayload* payload = JobPayload::Create(std::forward<Fn>(work_func), priority);
    EnqueuePayload(payload);
}

template <typename Fn>
    requires std::invocable<Fn>
JobHandle JobSystem::Submit(Fn&& work_func, EJobPriority priority)
{
    JobHandle handle = JobHandle::Create(1);

    JobPayload* payload = JobPayload::Create(std::forward<Fn>(work_func), priority);
    payload->completion_counter = handle.GetSharedCounter();

    EnqueuePayload(payload);
    return handle;
}

template <typename Fn>
    requires std::invocable<Fn>
JobHandle JobSystem::Submit(
    Fn&& work_func,
    ArrayView<const JobHandle> dependencies,
    EJobPriority priority
)
{
    JobHandle handle = JobHandle::Create(1);

    JobPayload* payload = JobPayload::Create(std::forward<Fn>(work_func), priority);
    payload->completion_counter = handle.GetSharedCounter();

    // 유효한 의존성 개수를 카운트
    const usize dep_count = std::ranges::count_if(dependencies, [](const JobHandle& dep)
    {
        return dep.IsValid();
    });

    // 의존성이 없으면 바로 전송(Enqueue)
    if (dep_count == 0)
    {
        EnqueuePayload(payload);
        return handle;
    }

    // 가드 카운트(+1): 등록 도중 모든 의존성이 해소되어도 payload가 조기 전송되는 것을 방지
    payload->pending_deps.store(dep_count + 1, std::memory_order_relaxed);

    // 각 의존성의 JobCounter에 Waiter를 등록
    for (const JobHandle& dep : dependencies)
    {
        if (!dep)
        {
            continue;
        }

        // dep의 Counter가 완료될 때, 이 JobPayload의 대기 카운트를 감소시키는 콜백 등록
        Optional<UniqueFunction<void()>> result = dep.GetCounter()->AddWaiter([this, payload]
        {
            // 가장 마지막 의존성이 payload를 Deque에 추가
            if (payload->pending_deps.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                EnqueuePayload(payload);
            }
        });

        // 등록 시점에 이미 선행 작업이 완료된 경우, 반환된 콜백을 바로 Invoke()
        if (result.HasValue())
        {
            result->Invoke();
        }
    }

    // 가드 카운트를 해제
    if (payload->pending_deps.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        EnqueuePayload(payload);
    }

    return handle;
}

template <typename Fn>
    requires std::invocable<Fn, usize>
JobHandle JobSystem::ParallelFor(usize in_count, usize batch_size, Fn work_func, EJobPriority priority)
{
    if (in_count == 0)
    {
        return {};
    }

    const usize batch_count = (in_count + batch_size - 1) / batch_size;
    JobHandle handle = JobHandle::Create(batch_count);

    for (usize batch = 0; batch < batch_count; ++batch)
    {
        const usize begin = batch * batch_size;
        const usize end = std::min(begin + batch_size, in_count);

        JobPayload* payload = JobPayload::Create([work = work_func, begin, end]
        {
            for (usize i = begin; i < end; ++i)
            {
                work(i);
            }
        }, priority);
        payload->completion_counter = handle.GetSharedCounter();

        EnqueuePayload(payload);
    }

    return handle;
}
} // namespace se
