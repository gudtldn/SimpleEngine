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
/**
 * Atomic 의존성 카운터 클래스
 *
 * count가 0에 도달하면 등록된 모든 Waiter(코루틴/콜백)를 깨웁니다.
 * Job간 의존성 해소를 위한 동기화 프리미티브입니다.
 */
class SE_CORE_API JobCounter
{
public:
    explicit JobCounter(usize in_initial_count);
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
     * @param in_handle 완료 시 resume할 코루틴 핸들
     * @return 등록에 성공 시 NullOpt 반환, 실패 시(카운터가 완료된 경우) in_handle을 그대로 반환
     */
    Optional<std::coroutine_handle<>> AddWaiter(std::coroutine_handle<> in_handle);

    /**
     * 콜백 Waiter를 등록합니다.
     * @param in_callback 완료 시 호출할 함수
     * @return 등록에 성공 시 NullOpt 반환, 실패 시(카운터가 완료된 경우) Callback의 소유권을 다시 반환
     */
    [[nodiscard]] Optional<UniqueFunction<void()>> AddWaiter(UniqueFunction<void()>&& in_callback);

private:
    struct WaiterNode
    {
        WaiterNode* next = nullptr;
        std::coroutine_handle<> coroutine; // 값이 존재하면 코루틴 resume
        UniqueFunction<void()> callback;   // 코루틴이 null이면 콜백 호출

        void* operator new(usize size);
        void operator delete(void* ptr);
    };

    /**
     * 완료 상태를 나타내는 센티널 포인터를 가져옵니다.
     * @warning 절대로 역참조를 해서는 안됩니다. (UB)
     */
    static WaiterNode* CompletedSentinel();

    /** Waiter 리스트를 순회하며 모두 깨웁니다. */
    static void NotifyWaiters(const WaiterNode* in_list);

private:
    // False Sharing 방지를 위한 캐시라인 정렬

    /** 카운터 값. 0이면 완료 상태 */
    alignas(SE_CACHE_LINE) std::atomic<usize> count;

    /** Waiter 리스트. waiters == CompletedSentinel()이면 완료 상태 */
    alignas(SE_CACHE_LINE) std::atomic<WaiterNode*> waiters = nullptr;
};


/**
 * 제출된 Job의 완료 상태를 추적하는 핸들
 */
class SE_CORE_API JobHandle
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

    /**
     * 작업이 완료될 때까지 현재 스레드를 블로킹합니다.
     * @note 추후 Job Stealing 방식의 워커 스레드 대기 로직으로 확장될 예정입니다.
     */
    void Wait() const;

    /** 내부 카운터 포인터를 반환합니다. (nullptr일 수 있음) */
    [[nodiscard]] FORCE_INLINE JobCounter* GetCounter() const { return counter.get(); }

    [[nodiscard]] explicit operator bool() const { return IsValid(); }
    Awaiter operator co_await() const { return { GetCounter() }; }

private:
    std::shared_ptr<JobCounter> counter;
};
} // namespace se
