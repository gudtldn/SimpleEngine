// ReSharper disable CppRedundantTypenameKeyword
#pragma once

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <ranges>


namespace se
{
template <typename T, typename Pred, typename Allocator>
FlatSet<T, Pred, Allocator>::FlatSet(std::initializer_list<ValueType> init_list)
    : internal_array(init_list)
{
    std::sort(internal_array.begin(), internal_array.end(), compare);
    // 중복 제거
    auto unique_end = std::unique(internal_array.begin(), internal_array.end());
    internal_array.Truncate(std::distance(internal_array.begin(), unique_end));
}

template <typename T, typename Pred, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
FlatSet<T, Pred, Allocator>::FlatSet(It first, Sent last)
    : internal_array(first, last)
{
    std::sort(internal_array.begin(), internal_array.end(), compare);
    auto unique_end = std::unique(internal_array.begin(), internal_array.end());
    internal_array.Truncate(std::distance(internal_array.begin(), unique_end));
}

template <typename T, typename Pred, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
FlatSet<T, Pred, Allocator> FlatSet<T, Pred, Allocator>::FromRange(Rng&& range)
{
    return FlatSet(std::ranges::begin(range), std::ranges::end(range));
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::SizeType FlatSet<T, Pred, Allocator>::Len() const noexcept
{
    return internal_array.Len();
}

template <typename T, typename Pred, typename Allocator>
bool FlatSet<T, Pred, Allocator>::IsEmpty() const noexcept
{
    return internal_array.IsEmpty();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::SizeType FlatSet<T, Pred, Allocator>::Capacity() const noexcept
{
    return internal_array.Capacity();
}

template <typename T, typename Pred, typename Allocator>
void FlatSet<T, Pred, Allocator>::Clear() noexcept
{
    internal_array.Clear();
}

template <typename T, typename Pred, typename Allocator>
bool FlatSet<T, Pred, Allocator>::Insert(const ValueType& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), value, compare);
    if (it != internal_array.end() && !compare(value, *it))
    {
        return false;
    }

    internal_array.Insert(std::distance(internal_array.begin(), it), value);
    return true;
}

template <typename T, typename Pred, typename Allocator>
bool FlatSet<T, Pred, Allocator>::Insert(ValueType&& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), value, compare);
    if (it != internal_array.end() && !compare(value, *it))
    {
        return false;
    }

    internal_array.Insert(std::distance(internal_array.begin(), it), std::move(value));
    return true;
}

template <typename T, typename Pred, typename Allocator>
template <typename ... Args>
bool FlatSet<T, Pred, Allocator>::Emplace(Args&&... args)
{
    ValueType value(std::forward<Args>(args)...);
    return Insert(std::move(value));
}

template <typename T, typename Pred, typename Allocator>
bool FlatSet<T, Pred, Allocator>::Remove(const ValueType& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), value, compare);
    if (it != internal_array.end() && !compare(value, *it))
    {
        internal_array.RemoveAt(std::distance(internal_array.begin(), it));
        return true;
    }
    return false;
}

template <typename T, typename Pred, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
typename FlatSet<T, Pred, Allocator>::SizeType FlatSet<T, Pred, Allocator>::RemoveIf(Predicate&& pred)
{
    return internal_array.RemoveIf(std::forward<Predicate>(pred));
}

template <typename T, typename Pred, typename Allocator>
bool FlatSet<T, Pred, Allocator>::Contains(const ValueType& value) const
{
    return std::binary_search(internal_array.begin(), internal_array.end(), value, compare);
}

template <typename T, typename Pred, typename Allocator>
const Array<T, Allocator>& FlatSet<T, Pred, Allocator>::GetArray() const noexcept
{
    return internal_array;
}

template <typename T, typename Pred, typename Allocator>
Array<typename FlatSet<T, Pred, Allocator>::ValueType> FlatSet<T, Pred, Allocator>::ToArray() const
{
    return internal_array;
}

template <typename T, typename Pred, typename Allocator>
void FlatSet<T, Pred, Allocator>::Reserve(SizeType new_capacity)
{
    internal_array.Reserve(new_capacity);
}

template <typename T, typename Pred, typename Allocator>
void FlatSet<T, Pred, Allocator>::ShrinkToFit()
{
    internal_array.ShrinkToFit();
}

template <typename T, typename Pred, typename Allocator>
void FlatSet<T, Pred, Allocator>::Swap(FlatSet& other) noexcept
{
    internal_array.Swap(other.internal_array);
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::IteratorType FlatSet<T, Pred, Allocator>::begin() noexcept
{
    return internal_array.begin();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::IteratorType FlatSet<T, Pred, Allocator>::end() noexcept
{
    return internal_array.end();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::ConstIteratorType FlatSet<T, Pred, Allocator>::begin() const noexcept
{
    return internal_array.begin();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::ConstIteratorType FlatSet<T, Pred, Allocator>::end() const noexcept
{
    return internal_array.end();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::ReverseIteratorType FlatSet<T, Pred, Allocator>::rbegin() noexcept
{
    return internal_array.rbegin();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::ReverseIteratorType FlatSet<T, Pred, Allocator>::rend() noexcept
{
    return internal_array.rend();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::ConstReverseIteratorType FlatSet<T, Pred, Allocator>::rbegin() const noexcept
{
    return internal_array.rbegin();
}

template <typename T, typename Pred, typename Allocator>
typename FlatSet<T, Pred, Allocator>::ConstReverseIteratorType FlatSet<T, Pred, Allocator>::rend() const noexcept
{
    return internal_array.rend();
}
} // namespace se
