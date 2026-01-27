// ReSharper disable CppRedundantTypenameKeyword
#pragma once
#include <algorithm>
#include <functional>
#include <initializer_list>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"


namespace se
{
/**
 * 우선순위 큐 컨테이너입니다.
 * @details 내부적으로 이진 힙(Binary Heap)으로 구현되며,
 *          가장 우선순위가 높은 요소에 대한 빠른 접근과 추출을 보장합니다.
 *          기본적으로 최대 힙(Max-Heap)으로 동작합니다.
 *
 * @tparam T 요소의 타입.
 * @tparam Container 내부적으로 사용할 시퀀스 컨테이너의 타입.
 * @tparam Compare 우선순위를 비교하는 함수의 타입. (기본값: a < b)
 */
template <
    typename T,
    typename Container = Array<T>,
    typename Compare = std::less<typename Container::ValueType>>
class PriorityQueue
{
public:
    using ContainerType = Container;
    using ValueType = ContainerType::ValueType;
    using SizeType = ContainerType::SizeType;
    using CompareType = Compare;

public:
    PriorityQueue() = default;
    PriorityQueue(std::initializer_list<ValueType> init_list);

    /**
     * 이터레이터 범위를 사용하여 힙을 구성합니다.
     * @param first 범위의 시작 이터레이터.
     * @param last 범위의 끝 이터레이터.
     */
    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, T>
    PriorityQueue(It first, Sent last);

public:
    /** 큐가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** 큐에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** 큐의 모든 요소를 제거하여 비웁니다. */
    void Clear() noexcept;

    /**
     * 큐에 새로운 요소를 추가합니다.
     * @param value 추가할 요소.
     */
    void Push(const ValueType& value);
    void Push(ValueType&& value);

    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    void PushRange(Rng&& range);

    /**
     * 큐에 새로운 요소를 내부 생성(emplace)하여 추가합니다.
     * @tparam Args 요소의 생성자에 전달할 인수들의 타입.
     * @param args 요소의 생성자에 전달할 인수들.
     */
    template <typename... Args>
    void Emplace(Args&&... args);

    /**
     * 큐에서 우선순위가 가장 높은 요소를 제거하지 않고 확인합니다.
     * @return 우선순위가 가장 높은 요소에 대한 Optional 참조. 큐가 비어있으면 nullopt입니다.
     */
    [[nodiscard]] Optional<const ValueType&> Peek() const;

    /**
     * 큐에서 우선순위가 가장 높은 요소를 제거하고, 그 값을 반환합니다.
     * @return 제거된 요소의 값을 담은 Optional. 큐가 비어있었으면 nullopt입니다.
     */
    Optional<ValueType> Pop();

    /**
     * 내부 컨테이너를 복사하여 반환합니다.
     * @note 반환된 Array는 힙 속성을 가지지만 정렬되어 있지는 않습니다.
     */
    [[nodiscard]] ContainerType ToUnderlyingContainer() const;

    void Swap(PriorityQueue& other) noexcept;

public:
    friend void swap(PriorityQueue& lhs, PriorityQueue& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    ContainerType container; // 내부 컨테이너
    CompareType comp;        // 비교 함수 객체
};

template <typename T, typename Container, typename Compare>
PriorityQueue<T, Container, Compare>::PriorityQueue(std::initializer_list<ValueType> init_list)
    : container(init_list)
    , comp{}
{
    std::ranges::make_heap(container, comp);
}

template <typename T, typename Container, typename Compare>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
PriorityQueue<T, Container, Compare>::PriorityQueue(It first, Sent last)
    : container(first, last)
    , comp{}
{
    std::ranges::make_heap(container, comp);
}

template <typename T, typename Container, typename Compare>
bool PriorityQueue<T, Container, Compare>::IsEmpty() const noexcept
{
    return container.IsEmpty();
}

template <typename T, typename Container, typename Compare>
PriorityQueue<T, Container, Compare>::SizeType PriorityQueue<T, Container, Compare>::Len() const noexcept
{
    return container.Len();
}

template <typename T, typename Container, typename Compare>
void PriorityQueue<T, Container, Compare>::Clear() noexcept
{
    container.Clear();
}

template <typename T, typename Container, typename Compare>
void PriorityQueue<T, Container, Compare>::Push(const ValueType& value)
{
    container.Push(value);
    std::ranges::push_heap(container, comp);
}

template <typename T, typename Container, typename Compare>
void PriorityQueue<T, Container, Compare>::Push(ValueType&& value)
{
    container.Push(std::move(value));
    std::ranges::push_heap(container, comp);
}

template <typename T, typename Container, typename Compare>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
void PriorityQueue<T, Container, Compare>::PushRange(Rng&& range)
{
    container.PushRange(std::forward<Rng>(range));
    std::ranges::make_heap(container, comp);
}

template <typename T, typename Container, typename Compare>
template <typename... Args>
void PriorityQueue<T, Container, Compare>::Emplace(Args&&... args)
{
    container.Emplace(std::forward<Args>(args)...);
    std::ranges::push_heap(container, comp);
}

template <typename T, typename Container, typename Compare>
Optional<const typename PriorityQueue<T, Container, Compare>::ValueType&> PriorityQueue<T, Container, Compare>::Peek() const
{
    return container.Front();
}

template <typename T, typename Container, typename Compare>
Optional<typename PriorityQueue<T, Container, Compare>::ValueType> PriorityQueue<T, Container, Compare>::Pop()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    std::ranges::pop_heap(container, comp);
    return container.Pop();
}

template <typename T, typename Container, typename Compare>
PriorityQueue<T, Container, Compare>::ContainerType PriorityQueue<T, Container, Compare>::ToUnderlyingContainer() const
{
    return container;
}

template <typename T, typename Container, typename Compare>
void PriorityQueue<T, Container, Compare>::Swap(PriorityQueue& other) noexcept
{
    container.Swap(other.container);
    std::swap(comp, other.comp);
}
}  // namespace se
