#pragma once

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <atomic>
#include <memory>
#include <type_traits>


namespace se
{
class JobCounter;

/**
 * 타입이 소거된(Type-Erased) Job 실행 단위
 *
 * 주어진 람다/함수 객체를 감싸며, 인라인 SBO 버퍼(48바이트)를 통해 대부분의 캡처를 힙 할당 없이 저장합니다.
 * SBO 용량을 초과하는 경우에만 JobAllocator에서 추가 블록을 할당합니다.
 */
struct JobPayload
{
    /** 인라인 SBO 버퍼 용량 (바이트). [this, a, b, c, d, e] 수준의 캡처를 커버합니다. */
    static constexpr usize SBO_CAPACITY = 48;

    /** SBO 버퍼 정렬 단위 */
    static constexpr usize SBO_ALIGNMENT = 16;

public:
    // ── 타입 소거된 함수 포인터 ─────────────────────────────────────

    /** 저장된 callable을 호출합니다. */
    void (*invoke_fn)(void* storage) = nullptr;

    /** 저장된 callable을 소멸합니다. */
    void (*destroy_fn)(void* storage) = nullptr;

public:
    // ── 저장 공간 ──────────────────────────────────────────────────

    /** SBO 인라인 버퍼. 작은 callable은 여기에 placement new됩니다. */
    alignas(SBO_ALIGNMENT) uint8 inline_storage[SBO_CAPACITY];

    /** SBO 초과 시 JobAllocator에서 할당된 외부 블록 (nullptr이면 인라인) */
    void* heap_block = nullptr;

public:
    // ── 스케줄링 메타데이터 ────────────────────────────────────────

    /** 이 Job의 우선순위 */
    EJobPriority priority = EJobPriority::Normal;

    /** 이 Job 실행 완료 시 Decrement할 카운터 (nullptr이면 독립 실행) */
    JobCounter* completion_counter = nullptr;

    /** 미해소 의존성 개수. 0이 되면 워커 Deque에 삽입됩니다. */
    std::atomic<usize> pending_deps = 0;

    /** 글로벌 인박스(Treiber Stack) 연결용 포인터 */
    JobPayload* next_pending = nullptr;

public:
    ~JobPayload()
    {
        if (destroy_fn)
        {
            destroy_fn(GetStorage());
            destroy_fn = nullptr;
        }

        if (heap_block)
        {
            JobAllocator::Free(heap_block);
            heap_block = nullptr;
        }
    }

    // 복사 & 이동 금지 (포인터로만 전달)
    JobPayload(const JobPayload&) = delete;
    JobPayload& operator=(const JobPayload&) = delete;
    JobPayload(JobPayload&&) = delete;
    JobPayload& operator=(JobPayload&&) = delete;

    void* operator new(usize in_size)
    {
        return JobAllocator::Allocate(in_size);
    }

    void operator delete(void* in_ptr)
    {
        JobAllocator::Free(in_ptr);
    }

public:
    /** callable이 저장된 위치의 포인터를 반환합니다. */
    [[nodiscard]] FORCE_INLINE void* GetStorage() noexcept
    {
        return heap_block ? heap_block : static_cast<void*>(inline_storage);
    }

    /** 저장된 callable을 호출합니다. */
    FORCE_INLINE void Invoke()
    {
        if (invoke_fn)
        {
            invoke_fn(GetStorage());
        }
    }

public:
    /**
     * 주어진 callable로 JobPayload를 생성합니다.
     *
     * callable의 크기가 SBO_CAPACITY 이내이면 인라인 버퍼에 placement new,
     * 초과하면 JobAllocator에서 외부 블록을 할당하여 저장합니다.
     *
     * @tparam Fn callable 타입 (람다, 함수 객체 등)
     * @param in_func 저장할 callable
     * @param in_priority Job 우선순위
     * @return JobAllocator에서 할당된 JobPayload 포인터
     */
    template <typename Fn>
    static JobPayload* Create(Fn&& in_func, EJobPriority in_priority = EJobPriority::Normal)
    {
        using DecayedFn = std::decay_t<Fn>;

        JobPayload* payload = new JobPayload{};
        payload->priority = in_priority;

        // 타입 소거된 호출/소멸 함수 포인터 설정
        payload->invoke_fn = [](void* storage)
        {
            std::invoke(*static_cast<DecayedFn*>(storage));
        };

        payload->destroy_fn = [](void* storage)
        {
            std::destroy_at(static_cast<DecayedFn*>(storage));
        };

        // SBO 조건: 크기와 정렬이 인라인 버퍼에 맞는지 확인
        if constexpr (sizeof(DecayedFn) <= SBO_CAPACITY && alignof(DecayedFn) <= SBO_ALIGNMENT)
        {
            // SBO: 인라인 버퍼에 placement new
            std::construct_at(
                reinterpret_cast<DecayedFn*>(payload->inline_storage),
                std::forward<Fn>(in_func)
            );
        }
        else
        {
            // 외부 블록: JobAllocator에서 할당 후 placement new
            void* block = JobAllocator::Allocate(sizeof(DecayedFn));
            payload->heap_block = std::construct_at(
                static_cast<DecayedFn*>(block),
                std::forward<Fn>(in_func)
            );
        }

        return payload;
    }

private:
    JobPayload() = default;
};
} // namespace se
