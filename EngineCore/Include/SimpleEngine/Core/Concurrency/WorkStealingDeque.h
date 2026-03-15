#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"

#include <atomic>
#include <bit>
#include <utility>


namespace se
{
/**
 * Chase-Lev Work-Stealing Deque
 *
 * 단일 소유자(Owner)가 작업을 생산/소비하고, 다른 스레드(Thief)가 작업을 훔쳐갈 수 있는 Lock-Free 양방향 큐입니다.
 * - Owner (LIFO): 최근 작업을 우선 처리(Pop)하여 캐시 지역성(Locality)을 극대화합니다.
 * - Thief (FIFO): 가장 오래된 작업을 훔쳐(Steal)가서 작업의 병렬 실행 효율을 높입니다.
 *
 * [Reference]
 * 1. 기본 구조: "Dynamic Circular Work-Stealing Deque" (Chase & Lev, 2005)
 *    https://www.dre.vanderbilt.edu/~schmidt/PDF/work-stealing-dequeue.pdf
 *
 * 2. 메모리 모델 최적화: "Correct and Efficient Work-Stealing for Weak Memory Models" (Lê et al., 2013)
 *    https://inria.hal.science/hal-00802885/document
 *
 * [Memory Safety]
 * - Sequential Consistency: Pop과 Steal이 마지막 남은 한 개의 항목을 두고 경쟁할 때,
 *   Store-Load 재정렬을 방지하기 위해 강한 메모리 배리어(seq_cst)를 사용합니다.
 * - Circular Buffer: 인덱스(top/bottom)는 무한히 증가하는 64비트 정수이며, 실제 버퍼 접근 시 마스크 연산으로 래핑합니다.
 *   (버퍼 크기가 2의 거듭제곱일 때, 나머지 연산(%) 대신 & (size - 1) 비트 연산으로 빠르게 순회하는 원리)
 *
 * @tparam T Deque에 저장할 요소 타입 (T must be trivially copyable)
 */
template <typename T>
class WorkStealingDeque
{
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable (e.g. pointer) for thread-safe Steal/Pop reads");

public:
    /** @param in_initial_capacity_log2 초기 용량 (2의 거듭제곱 지수) */
    explicit WorkStealingDeque(usize in_initial_capacity_log2 = 10);
    ~WorkStealingDeque();

    // 복사 & 이동 금지
    WorkStealingDeque(const WorkStealingDeque&) = delete;
    WorkStealingDeque& operator=(const WorkStealingDeque&) = delete;
    WorkStealingDeque(WorkStealingDeque&&) = delete;
    WorkStealingDeque& operator=(WorkStealingDeque&&) = delete;

public:
    /** Deque 하단(Bottom)에 항목을 추가합니다. (Owner 전용) */
    void Push(T in_item);

    /**
     * Deque 하단(Bottom)에서 가장 최근 항목을 꺼냅니다. (Owner 전용)
     * @note Steal과 경합 시 실패할 수 있습니다.
     */
    Optional<T> Pop();

    /**
     * Deque 상단(Top)에서 가장 오래된 항목을 훔칩니다. (Thief 가능)
     * @note 다른 Thief와 경합 시 실패할 수 있습니다.
     */
    Optional<T> Steal();

    /**
     * 대략적인 현재 작업 수입니다. (Thief 가능)
     * @note 동시성으로 인해 정확하지 않을 수 있습니다.
     */
    [[nodiscard]] usize ApproxSize() const;

    /** Deque가 비어있는지 확인합니다. (Thief 가능) */
    [[nodiscard]] bool IsEmpty() const;

private:
    /**
     * 동적 리사이즈 가능한 원형 버퍼
     * capacity는 항상 2의 거듭제곱이며, 비트마스크로 인덱스를 래핑합니다.
     */
    struct CircularBuffer
    {
        int64 capacity; // 2^log2_capacity
        int64 mask;     // capacity - 1
        T* storage;

        explicit CircularBuffer(usize in_log2_cap)
            : capacity(static_cast<int64>(1) << in_log2_cap)
            , mask(capacity - 1)
            , storage(new T[static_cast<usize>(capacity)])
        {
        }

        ~CircularBuffer()
        {
            delete[] storage;
        }

        T Get(int64 in_index) const
        {
            return storage[in_index & mask];
        }

        void Set(int64 in_index, T in_value)
        {
            storage[in_index & mask] = std::move(in_value);
        }

        /** 새 버퍼를 할당하고 기존 [top, bottom) 범위를 복사하여 반환합니다. */
        CircularBuffer* Grow(int64 in_top, int64 in_bottom) const
        {
            // capacity의 log2를 재계산
            const usize log2_cap = std::bit_width(static_cast<usize>(capacity)) - 1;

            CircularBuffer* new_buf = new CircularBuffer(log2_cap + 1);
            for (int64 i = in_top; i < in_bottom; ++i)
            {
                new_buf->Set(i, Get(i));
            }
            return new_buf;
        }
    };

    /** 사용이 끝난 이전 버퍼를 가비지 목록에 등록합니다. (Owner 스레드 전용) */
    void RetireBuffer(CircularBuffer* in_old_buffer);

private:
    // False Sharing 방지를 위해 각 인덱스들을 독립된 캐시 라인에 배치

    /**
     * top과 bottom이 int64인 이유는,
     * Pop() 연산 시 bottom이 top보다 일시적으로 작아져 음수 방향의 비교(예: 0 <= -1)가 발생하기 때문
     */
    alignas(SE_CACHE_LINE) std::atomic<int64> top = 0;
    alignas(SE_CACHE_LINE) std::atomic<int64> bottom = 0;
    alignas(SE_CACHE_LINE) std::atomic<CircularBuffer*> buffer = nullptr;

    /** Grow로 생성된 이전 버퍼들을 보관하여, 소멸자에서 일괄 해제합니다. (Epoch-based reclamation 단순화) */
    Array<CircularBuffer*> garbage{};
};

template <typename T>
WorkStealingDeque<T>::WorkStealingDeque(usize in_initial_capacity_log2)
{
    SE_ASSERT( // NOLINT(*-simplify-boolean-expr)
        in_initial_capacity_log2 > 0 && in_initial_capacity_log2 < 30,
        "Capacity log2 must be in (0, 30), got {}", in_initial_capacity_log2
    );

    buffer.store(new CircularBuffer(in_initial_capacity_log2), std::memory_order_relaxed);
}

template <typename T>
WorkStealingDeque<T>::~WorkStealingDeque()
{
    delete buffer.load(std::memory_order_relaxed);

    for (const CircularBuffer* garbage_buf : garbage)
    {
        delete garbage_buf;
    }
}

template <typename T>
void WorkStealingDeque<T>::Push(T in_item)
{
    const int64 bottom_idx = bottom.load(std::memory_order_relaxed);
    const int64 top_idx = top.load(std::memory_order_acquire);
    CircularBuffer* buf = buffer.load(std::memory_order_relaxed);

    // capacity 부족 시 리사이즈
    if (bottom_idx - top_idx >= buf->capacity)
    {
        CircularBuffer* old_buf = buf;
        buf = buf->Grow(top_idx, bottom_idx);
        buffer.store(buf, std::memory_order_release);
        RetireBuffer(old_buf);
    }

    buf->Set(bottom_idx, std::move(in_item));

    // Steal()이 안전하게 데이터를 읽을 수 있도록 bottom 갱신 전에 .store()의 메모리 가시성을 보장
    bottom.store(bottom_idx + 1, std::memory_order_release);
}

template <typename T>
Optional<T> WorkStealingDeque<T>::Pop()
{
    const int64 bottom_idx = bottom.load(std::memory_order_relaxed) - 1;
    CircularBuffer* buf = buffer.load(std::memory_order_relaxed);
    bottom.store(bottom_idx, std::memory_order_relaxed);

    // Pop()과 Steal()이 마지막 항목을 동시에 가져가지 않도록 Store-Load 재정렬을 방지
    std::atomic_thread_fence(std::memory_order_seq_cst);

    const int64 top_idx = top.load(std::memory_order_relaxed);

    if (top_idx <= bottom_idx)
    {
        // Deque에 항목이 1개 이상
        T item = buf->Get(bottom_idx);

        if (top_idx == bottom_idx)
        {
            // 마지막 항목 - Steal과 경합 가능
            int64 expected_top = top_idx;
            if (!top.compare_exchange_strong(expected_top, top_idx + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                // Steal이 먼저 가져감
                bottom.store(top_idx + 1, std::memory_order_relaxed);
                return NullOpt;
            }
            bottom.store(top_idx + 1, std::memory_order_relaxed);
        }

        return item;
    }

    // Deque가 비어있음
    bottom.store(top_idx, std::memory_order_relaxed);
    return NullOpt;
}

template <typename T>
Optional<T> WorkStealingDeque<T>::Steal()
{
    const int64 top_idx = top.load(std::memory_order_acquire);

    // Push된 최신 bottom 값을 놓치지 않도록 top과 bottom load 사이의 Store-Load 재정렬을 방지
    std::atomic_thread_fence(std::memory_order_seq_cst);

    const int64 bottom_idx = bottom.load(std::memory_order_acquire);
    if (top_idx >= bottom_idx)
    {
        // Deque가 비어있음
        return NullOpt;
    }

    CircularBuffer* buf = buffer.load(std::memory_order_acquire);
    T item = buf->Get(top_idx);

    // CAS로 top을 전진시킨다.
    // 실패하면 다른 Worker가 먼저 훔친 것이므로 포기한다.
    int64 expected_top = top_idx;
    if (!top.compare_exchange_strong(expected_top, top_idx + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
        return NullOpt;
    }

    return item;
}

template <typename T>
usize WorkStealingDeque<T>::ApproxSize() const
{
    const int64 bottom_idx = bottom.load(std::memory_order_relaxed);
    const int64 top_idx = top.load(std::memory_order_relaxed);
    const int64 size = bottom_idx - top_idx;

    return static_cast<usize>(std::max<int64>(size, 0));
}

template <typename T>
bool WorkStealingDeque<T>::IsEmpty() const
{
    // Chase-Lev Deque에서 bottom이 top보다 작거나 같으면 큐가 비어있음을 의미
    return bottom.load(std::memory_order_relaxed) <= top.load(std::memory_order_relaxed);;
}

template <typename T>
void WorkStealingDeque<T>::RetireBuffer(CircularBuffer* in_old_buffer)
{
    // Owner 스레드만 접근하므로 Lock 불필요
    garbage.Push(in_old_buffer);
}
} // namespace se
