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
     * 작업을 워커 스레드에 제출합니다.
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
     * 선행 작업 완료 후 실행될 의존성 작업을 제출합니다.
     *
     * 모든 선행 작업(deps)이 완료되면 자동으로 워커 Deque에 삽입됩니다.
     * JobCounter의 Waiter 메커니즘을 활용하여 원자적으로 의존성을 해소합니다.
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
     * 메인 스레드에서 실행될 작업을 제출합니다. (모든 스레드에서 호출 가능)
     * @param work_func 메인 스레드에서 실행할 작업
     */
    void SubmitMain(UniqueFunction<void()>&& work_func);

    /**
     * 범위를 배치 단위로 분할하여 병렬 실행합니다.
     *
     * @tparam Fn void(usize index)로 호출 가능한 callable 타입
     * @param in_count 총 반복 횟수
     * @param batch_size 배치당 반복 횟수
     * @param work_func 각 인덱스에 대해 호출될 callable
     * @param priority 실행 우선순위
     * @return 모든 배치 완료를 추적하는 핸들
     */
    template <typename Fn>
        requires std::invocable<Fn, usize>
    JobHandle ParallelFor(usize in_count, usize batch_size, Fn work_func, EJobPriority priority = EJobPriority::Normal);

    /**
     * 메인 스레드 큐에 쌓인 작업을 모두 실행합니다. (메인 스레드 전용)
     * @return 실행된 작업 수
     * @warning 반드시 메인 스레드에서만 호출해야 합니다.
     */
    usize ExecuteMainThreadJobs();

public:
    /**
     * 대기 중에 다른 Job을 하나 훔쳐서 실행합니다.
     * JobHandle::Wait()에서 CPU 유휴를 방지하기 위해 사용됩니다.
     * @return 작업을 실행했으면 true
     */
    bool TryExecuteOne();

    /** 워커 스레드 수를 반환합니다. */
    [[nodiscard]] usize GetWorkerCount() const;

private:
    /** JobPayload를 적절한 워커 Deque에 삽입하고 워커를 깨웁니다. */
    void SchedulePayload(JobPayload* payload);

    /** JobPayload를 실행하고 완료 카운터를 감소시킨 뒤 메모리를 해제합니다. */
    static void ExecutePayload(JobPayload* payload);

    /** 워커 스레드의 메인 루프 */
    void WorkerLoop(const std::stop_token& stoken, usize worker_index);

    /** 자신의 Deque에서 우선순위 순으로 Pop을 시도합니다. */
    JobPayload* TryPopLocal(usize worker_index);

    /** 다른 워커의 Deque에서 우선순위 순으로 Steal을 시도합니다. */
    JobPayload* TryStealFromOthers(usize thief_index);

    /** 글로벌 인박스에서 Payload를 하나 꺼냅니다. (Treiber Stack Pop) */
    JobPayload* TryPopGlobal();

    /** 현재 스레드의 워커 인덱스를 반환합니다. (비워커 스레드는 SIZE_MAX) */
    static usize& GetCurrentWorkerIndex();

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
JobHandle JobSystem::Submit(Fn&& work_func, EJobPriority priority)
{
    JobHandle handle = JobHandle::Create(1);

    JobPayload* payload = JobPayload::Create(std::forward<Fn>(work_func), priority);
    payload->completion_counter = handle.GetCounter();

    SchedulePayload(payload);
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
    payload->completion_counter = handle.GetCounter();

    // 유효한 의존성 개수를 카운트
    const usize dep_count = std::ranges::count_if(dependencies, [](const JobHandle& dep)
    {
        return dep.IsValid();
    });

    // 의존성이 없으면 바로 스케줄링
    if (dep_count == 0)
    {
        SchedulePayload(payload);
        return handle;
    }

    // 가드 카운트(+1): 등록 도중 모든 의존성이 해소되어도 payload가 조기 스케줄링되는 것을 방지하기 위함
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
                SchedulePayload(payload);
            }
        });

        // dep이 이미 완료된 상태인 경우, AddWaiter가 콜백을 반환하기 때문에 따로 Invoke()
        if (result.HasValue())
        {
            result->Invoke();
        }
    }

    // 가드 카운트를 해제
    if (payload->pending_deps.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        SchedulePayload(payload);
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
    JobCounter* counter = handle.GetCounter();

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
        payload->completion_counter = counter;

        SchedulePayload(payload);
    }

    return handle;
}
} // namespace se
