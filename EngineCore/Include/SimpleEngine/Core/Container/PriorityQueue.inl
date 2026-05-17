// ReSharper disable CppRedundantTypenameKeyword
#pragma once

#include <algorithm>
#include <utility>


namespace se
{
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
        return NullOpt;
    }
    std::ranges::pop_heap(container, comp);
    return container.Pop();
}

template <typename T, typename Container, typename Compare>
void PriorityQueue<T, Container, Compare>::Swap(PriorityQueue& other) noexcept
{
    container.Swap(other.container);
    std::swap(comp, other.comp);
}
} // namespace se
