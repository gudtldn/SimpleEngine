#pragma once

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <atomic>
#include <coroutine>
#include <memory>


namespace se
{
namespace detail
{
/**
 * JobTask<T>의 결과물을 보관하는 저장소
 * @tparam T 비동기 작업이 반환하는 실제 데이터 타입
 */
template <typename T>
struct JobSharedState
{
    Optional<T> result;
};
} // namespace detail

/**
 * Atomic 의존성 카운터 클래스
 *
 * count가 0에 도달하면 등록된 모든 Waiter(코루틴/콜백)를 깨웁니다.
 * Job간 의존성 해소를 위한 동기화 프리미티브입니다.
 */
class SE_CORE_API JobCounter
{
public:
    explicit JobCounter(usize initial_count);
    ~JobCounter();

    // 복사/이동 금지
    JobCounter(const JobCounter&) = delete;
    JobCounter& operator=(const JobCounter&) = delete;
    JobCounter(JobCounter&&) = delete;
    JobCounter& operator=(JobCounter&&) = delete;

public:
    /**
     * 카운터를 1 감소합니다. 0에 도달하면 모든 Waiter를 Notify합니다.
     * @warning count가 이미 0일 때 호출하면 UB
     */
    void Decrement();

    /** 카운터가 0인지 확인합니다. */
    [[nodiscard]] bool IsComplete() const;

    /** 현재 카운터의 값을 반환합니다. */
    [[nodiscard]] usize GetCount() const;

    /**
     * 코루틴 Waiter를 등록합니다.
     * @param handle 완료 시 resume할 코루틴 핸들
     * @return 등록에 성공 시 NullOpt 반환, 실패 시(카운터가 완료된 경우) in_handle을 그대로 반환
     */
    Optional<std::coroutine_handle<>> AddWaiter(std::coroutine_handle<> handle);

    /**
     * 콜백 Waiter를 등록합니다.
     * @param callback 완료 시 호출할 함수
     * @return 등록에 성공 시 NullOpt 반환, 실패 시(카운터가 완료된 경우) Callback의 소유권을 다시 반환
     */
    [[nodiscard]] Optional<UniqueFunction<void()>> AddWaiter(UniqueFunction<void()>&& callback);

private:
    struct WaiterNode
    {
        WaiterNode* next = nullptr;
        std::coroutine_handle<> coroutine; // 값이 존재하면 코루틴 resume
        UniqueFunction<void()> callback;   // 코루틴이 null이면 콜백 호출

        void* operator new(usize size);
        void operator delete(void* ptr);
    };

    /** 완료 상태를 나타내는 센티널 포인터를 가져옵니다. */
    static WaiterNode* CompletedSentinel();

    /** Waiter 리스트를 순회하며 모두 깨웁니다. */
    static void NotifyWaiters(WaiterNode* waiter_list);

private:
    // False Sharing 방지를 위한 캐시라인 정렬

    /** 카운터 값. 0이면 완료 상태 */
    alignas(SE_CACHE_LINE) std::atomic<usize> count;

    /** Waiter 리스트. waiters == CompletedSentinel()이면 완료 상태 */
    alignas(SE_CACHE_LINE) std::atomic<WaiterNode*> waiters = nullptr;
};


template <typename T>
class JobHandle;

/** 제출된 Job의 완료 상태를 추적하는 핸들 */
template <>
class SE_CORE_API JobHandle<void>
{
    /** JobHandle을 코루틴에서 대기하기 위한 Awaiter 구조체 */
    struct Awaiter
    {
        JobCounter* counter;

        bool await_ready() const noexcept // NOLINT(*-use-nodiscard)
        {
            return !counter || counter->IsComplete();
        }

        bool await_suspend(std::coroutine_handle<> in_handle) const // NOLINT(*-use-nodiscard)
        {
            // 등록 성공 시(NullOpt) true를 반환하여 코루틴을 중단(suspend)
            return counter && !counter->AddWaiter(in_handle).HasValue();
        }

        // ReSharper disable once CppMemberFunctionMayBeStatic
        void await_resume() const noexcept {}
    };

public:
    JobHandle() = default;

    explicit JobHandle(std::shared_ptr<JobCounter> in_counter)
        : counter(std::move(in_counter))
    {
    }

    /** 지정된 카운트를 가진 새 JobCounter를 생성하여 핸들을 반환합니다. */
    static JobHandle Create(usize in_count)
    {
        return JobHandle{ std::make_shared<JobCounter>(in_count) };
    }

    /**
     * 작업의 완료 여부를 확인합니다.
     * @return 카운터가 없거나 이미 0에 도달했다면 true
     */
    [[nodiscard]] bool IsComplete() const
    {
        return !counter || counter->IsComplete();
    }

    /** 유효한 카운터를 보유하고 있는지 확인합니다. */
    [[nodiscard]] bool IsValid() const { return counter != nullptr; }

    /** 작업이 완료될 때까지 현재 스레드를 블로킹합니다. */
    void Wait() const;

    /** 내부 카운터 포인터를 반환합니다. (nullptr일 수 있음) */
    [[nodiscard]] FORCE_INLINE JobCounter* GetCounter() const { return counter.get(); }

    /** 내부 카운터의 공유 소유권을 반환합니다. (JobPayload의 수명 보장용) */
    [[nodiscard]] FORCE_INLINE const std::shared_ptr<JobCounter>& GetSharedCounter() const { return counter; }

    [[nodiscard]] explicit operator bool() const { return IsValid(); }
    Awaiter operator co_await() const { return { GetCounter() }; }

private:
    std::shared_ptr<JobCounter> counter;
};

/**
 * 결과 값을 반환하는 JobTask의 상태를 추적하고 제어하는 핸들
 * @tparam T 비동기 작업이 완료된 후 반환할 데이터의 타입
 */
template <typename T>
class JobHandle
{
    /** JobHandle<T>를 코루틴 내부에서 대기하고 결과를 반환받기 위한 Awaiter 구조체 */
    struct Awaiter
    {
        JobHandle owner;

        bool await_ready() const noexcept // NOLINT(*-use-nodiscard)
        {
            return owner.IsComplete();
        }

        bool await_suspend(std::coroutine_handle<> in_handle) const // NOLINT(*-use-nodiscard)
        {
            JobCounter* counter = owner.inner_handle.GetCounter();
            return counter && !counter->AddWaiter(in_handle).HasValue();
        }

        T await_resume()
        {
            return owner.Get();
        }
    };

public:
    JobHandle() = default;

    JobHandle(JobHandle<void> in_handle, std::shared_ptr<detail::JobSharedState<T>> in_state)
        : inner_handle(std::move(in_handle)), state(std::move(in_state))
    {
    }

    /**
     * 작업의 완료 여부를 확인합니다.
     * @return 카운터가 없거나 이미 0에 도달했다면 true
     */
    [[nodiscard]] bool IsComplete() const
    {
        return inner_handle.IsComplete();
    }

    /** 유효한 핸들 및 저장소를 보유하고 있는지 확인합니다. */
    [[nodiscard]] bool IsValid() const
    {
        return inner_handle.IsValid() && state != nullptr;
    }

    /** 작업이 완료될 때까지 현재 스레드를 블로킹합니다. */
    void Wait() const
    {
        inner_handle.Wait();
    }

    /**
     * 비동기 작업의 완료를 보장하며, 최종 결과물을 반환합니다.
     * 작업이 진행 중일 경우 현재 스레드는 `TryExecuteOneJob()`을 통해 워크 스틸링(Stealing)을 수행합니다.
     *
     * @return 생산된 데이터 T (소유권이 이동되므로 1회만 호출 가능)
     * @warning 결과물이 비어있거나 소유권이 중복 추출될 경우 엔진 크래시가 발생합니다.
     */
    [[nodiscard]] T Get()
    {
        Wait();
        SE_ASSERT(state && state->result.HasValue(), "[JobSystem] Result value is missing.");
        return std::move(state->result).Value();
    }

    operator JobHandle<void>() const noexcept { return inner_handle; }
    Awaiter operator co_await() { return { *this }; }

private:
    /** 완료 카운팅 및 동기화를 전담하는 내부 핸들 */
    JobHandle<void> inner_handle;

    /** 프레임 수명과 분리되어 결과 데이터를 유지하는 공유 저장소 포인터 */
    std::shared_ptr<detail::JobSharedState<T>> state;
};
} // namespace se
