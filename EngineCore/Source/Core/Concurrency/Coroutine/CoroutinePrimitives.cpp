// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppDFAMemoryLeak
#include "SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h"
#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"

#include <algorithm>
#include <atomic>


namespace se
{
namespace
{
/** 코루틴 핸들을 JobSystem 워커에 fire-and-forget으로 스케줄링합니다. */
void ScheduleCoroutineResume(std::coroutine_handle<> handle)
{
    JobSystem::Get().Dispatch([h = handle]
    {
        h.resume();
    });
}

/** 코루틴 핸들을 메인 스레드 큐에 등록합니다. */
void ScheduleCoroutineResumeOnMain(std::coroutine_handle<> handle)
{
    JobSystem::Get().DispatchToMain([h = handle]
    {
        h.resume();
    });
}
} // namespace

bool ResumeOn::await_ready() const noexcept
{
    // TODO: 현재 스레드가 target이면 true를 반환하도록 최적화
    return false;
}

void ResumeOn::await_suspend(std::coroutine_handle<> handle) const
{
    switch (target)
    {
    case EJobThread::Main:
        ScheduleCoroutineResumeOnMain(handle);
        break;

    case EJobThread::Worker:
    case EJobThread::Auto:
        ScheduleCoroutineResume(handle);
        break;

    case EJobThread::IO:
        // TODO: SDL3_AsyncIO로 IO 전용 스레드 구현. 일단 Worker로 Fallback
        ScheduleCoroutineResume(handle);
        break;
    }
}


namespace
{
/**
 * WhenAll에서 공유하는 상태
 */
struct WhenAllState
{
    /**
     * 완료를 기다려야 하는 남은 작업(Job)의 개수
     * 이 값이 0에 도달하면 모든 작업이 끝난 것이므로 코루틴을 재개합니다.
     */
    std::atomic<usize> remaining;

    /** 모든 작업이 완료되었을 때 깨워줄(resume) 부모 코루틴의 핸들 */
    std::coroutine_handle<> handle;

    void* operator new(usize size) { return JobAllocator::Allocate(size); }
    void operator delete(void* ptr) { JobAllocator::Free(ptr); }
};

/**
 * WhenAny에서 공유하는 상태
 *
 * 2-bit phase 프로토콜로 콜백과 가드 사이의 resume 결정을 원자적으로 조율합니다
 *   - CALLBACK_WON (bit 0): 첫 번째 콜백이 resumed 경쟁에서 승리
 *   - GUARD_DONE   (bit 1): await_suspend가 모든 등록을 완료하고 반환 준비 완료
 *
 * 콜백이 이기면 CALLBACK_WON을 설정하고, GUARD_DONE이 이미 설정되어 있으면 직접 resume합니다.
 * 그렇지 않으면 await_suspend가 false를 반환하여 resume을 처리합니다.
 */
struct WhenAnyState
{
    // Phase Flag
    static constexpr uint8 WAITING = 0b00;
    static constexpr uint8 CALLBACK_WON = 0b01;
    static constexpr uint8 GUARD_DONE = 0b10;

    /** 콜백 등록(Guard)과 첫 번째 완료 콜백 간의 실행 순서를 조율하는 상태 값 */
    std::atomic<uint8> phase = WAITING;

    /** 여러 작업 중 정확히 단 하나(가장 먼저 끝난 작업)만 코루틴을 재개할 수 있도록 보장하는 초경량 Lock-Free 플래그 */
    std::atomic_flag resumed = ATOMIC_FLAG_INIT;

    /** 상태 객체의 수명을 관리하는 참조 카운트 */
    std::atomic<usize> ref_count = 0;

    /** 가장 먼저 작업이 완료되었을 때 깨워줄(resume) 부모 코루틴의 핸들 */
    std::coroutine_handle<> handle;

    void* operator new(usize size) { return JobAllocator::Allocate(size); }
    void operator delete(void* ptr) { JobAllocator::Free(ptr); }

    void AddRef()
    {
        ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    void Release()
    {
        // 마지막으로 참조를 놓는 스레드가 이 객체를 메모리 풀로 반환
        if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }
};
} // namespace


WhenAll::WhenAll(Array<JobHandle> in_handles)
    : handles(std::move(in_handles))
{
}

bool WhenAll::await_ready() const noexcept
{
    return std::ranges::all_of(handles, &JobHandle::IsComplete);
}

bool WhenAll::await_suspend(std::coroutine_handle<> awaiting)
{
    WhenAllState* state = new WhenAllState{};

    // 가드 카운트(+1)
    state->remaining.store(1, std::memory_order_relaxed);
    state->handle = awaiting;

    for (const JobHandle& handle : handles)
    {
        if (!handle.IsValid())
        {
            continue;
        }

        // 등록 전에 카운트 증가
        state->remaining.fetch_add(1, std::memory_order_relaxed);

        // 이미 완료된 handle은 콜백을 반환받고, 진행 중인 Job은 대기열에 등록(NullOpt)
        Optional<UniqueFunction<void()>> result = handle.GetCounter()->AddWaiter([state]
        {
            // 가장 마지막 handle이 coroutine.resume()을 호출
            if (state->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                const auto coroutine = state->handle;
                delete state;
                coroutine.resume();
            }
        });

        // 등록 시점에 이미 완료된 경우
        if (result.HasValue())
        {
            result->Invoke();
        }
    }

    // 가드 카운트를 해제
    if (state->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        delete state;
        return false; // 코루틴을 멈추지 않고 계속 진행
    }

    return true;
}

WhenAny::WhenAny(Array<JobHandle> in_handles)
    : handles(std::move(in_handles))
{
}

bool WhenAny::await_ready() const noexcept
{
    return std::ranges::any_of(handles, &JobHandle::IsComplete);
}

bool WhenAny::await_suspend(std::coroutine_handle<> awaiting)
{
    WhenAnyState* state = new WhenAnyState{};
    state->handle = awaiting;

    // 가드 카운트(+1)
    state->AddRef();

    for (const JobHandle& handle : handles)
    {
        if (!handle.IsValid())
        {
            continue;
        }

        // 이미 완료된 핸들을 발견하면 바로 return
        if (handle.IsComplete())
        {
            state->Release();
            return false; // 코루틴을 멈추지 않고 계속 진행
        }

        state->AddRef();

        Optional<UniqueFunction<void()>> result = handle.GetCounter()->AddWaiter([state]
        {
            // false를 반환하면, 이 콜백이 가장 먼저 진입하여 경쟁에서 승리했음을 의미
            if (!state->resumed.test_and_set(std::memory_order_acq_rel))
            {
                // 승리한 콜백으로서 CALLBACK_WON 상태를 원자적으로 기록
                const uint8 old_phase = state->phase.fetch_or(WhenAnyState::CALLBACK_WON, std::memory_order_acq_rel);

                // 만약 GUARD_DONE이 이미 설정되어 있다면, await_suspend의 등록 루프가 끝난 상태이므로 안전하게 직접 재개
                if (old_phase & WhenAnyState::GUARD_DONE)
                {
                    state->handle.resume();
                }
                // GUARD_DONE이 없다면 아직 await_suspend가 진행 중이므로, 대기(suspend) 대신 false 반환을 통해 호출자 측에서 재개하도록 함.
            }
            state->Release();
        });

        // AddWaiter 호출 시점에 이미 작업이 완료된 경우(레이스 발생), 반환된 콜백을 바로 Invoke()
        if (result.HasValue())
        {
            result->Invoke();
        }
    }

    // 모든 핸들의 등록이 완료되었으므로 가드를 해제하기 위해 GUARD_DONE 상태를 기록
    const uint8 old_phase = state->phase.fetch_or(
        WhenAnyState::GUARD_DONE, std::memory_order_acq_rel
    );

    // 가드 카운트를 해제
    state->Release();

    // 콜백이 이미 승리하여 완료되었으나 가드 상태로 인해 재개하지 못하고 대기했다면, 여기서 false를 반환하여 바로 resume
    if (old_phase & WhenAnyState::CALLBACK_WON)
    {
        // 코루틴을 멈추지 않고 계속 진행
        return false;
    }

    // 아직 완료된 콜백이 없다면 코루틴을 중단(Suspend)하고 대기 상태로 전환
    return true;
}
} // namespace se
