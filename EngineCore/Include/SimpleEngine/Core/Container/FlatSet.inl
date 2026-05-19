// ReSharper disable CppRedundantTypenameKeyword
#pragma once

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <ranges>


namespace se
{
template <typename T, typename Pred, typename Container>
FlatSet<T, Pred, Container>::FlatSet(std::initializer_list<ValueType> init_list)
    : internal_array(init_list)
{
    std::sort(internal_array.begin(), internal_array.end(), compare);

    // 중복 제거
    auto unique_end = std::unique(internal_array.begin(), internal_array.end());
    internal_array.Truncate(std::distance(internal_array.begin(), unique_end));
}

template <typename T, typename Pred, typename Container>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
FlatSet<T, Pred, Container>::FlatSet(It first, Sent last)
    : internal_array(first, last)
{
    std::sort(internal_array.begin(), internal_array.end(), compare);
    auto unique_end = std::unique(internal_array.begin(), internal_array.end());
    internal_array.Truncate(std::distance(internal_array.begin(), unique_end));
}

template <typename T, typename Pred, typename Container>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
FlatSet<T, Pred, Container> FlatSet<T, Pred, Container>::FromRange(Rng&& range)
{
    return FlatSet(std::ranges::begin(range), std::ranges::end(range));
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::SizeType FlatSet<T, Pred, Container>::Len() const noexcept
{
    return internal_array.Len();
}

template <typename T, typename Pred, typename Container>
bool FlatSet<T, Pred, Container>::IsEmpty() const noexcept
{
    return internal_array.IsEmpty();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::SizeType FlatSet<T, Pred, Container>::Capacity() const noexcept
{
    return internal_array.Capacity();
}

template <typename T, typename Pred, typename Container>
void FlatSet<T, Pred, Container>::Clear() noexcept
{
    internal_array.Clear();
}

template <typename T, typename Pred, typename Container>
bool FlatSet<T, Pred, Container>::Insert(const ValueType& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), value, compare);
    if (it != internal_array.end() && !compare(value, *it))
    {
        return false;
    }

    internal_array.Insert(std::distance(internal_array.begin(), it), value);
    return true;
}

template <typename T, typename Pred, typename Container>
bool FlatSet<T, Pred, Container>::Insert(ValueType&& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), value, compare);
    if (it != internal_array.end() && !compare(value, *it))
    {
        return false;
    }

    internal_array.Insert(std::distance(internal_array.begin(), it), std::move(value));
    return true;
}

template <typename T, typename Pred, typename Container>
template <typename... Args>
bool FlatSet<T, Pred, Container>::Emplace(Args&&... args)
{
    ValueType value(std::forward<Args>(args)...);
    return Insert(std::move(value));
}

template <typename T, typename Pred, typename Container>
bool FlatSet<T, Pred, Container>::Remove(const ValueType& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), value, compare);
    if (it != internal_array.end() && !compare(value, *it))
    {
        internal_array.RemoveAt(std::distance(internal_array.begin(), it));
        return true;
    }
    return false;
}

template <typename T, typename Pred, typename Container>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
typename FlatSet<T, Pred, Container>::SizeType FlatSet<T, Pred, Container>::RemoveIf(Predicate&& pred)
{
    return internal_array.RemoveIf(std::forward<Predicate>(pred));
}

template <typename T, typename Pred, typename Container>
bool FlatSet<T, Pred, Container>::Contains(const ValueType& value) const
{
    return std::binary_search(internal_array.begin(), internal_array.end(), value, compare);
}

template <typename T, typename Pred, typename Container>
const Container& FlatSet<T, Pred, Container>::GetArray() const noexcept
{
    return internal_array;
}

template <typename T, typename Pred, typename Container>
Array<typename FlatSet<T, Pred, Container>::ValueType> FlatSet<T, Pred, Container>::ToArray() const
{
    return internal_array;
}

template <typename T, typename Pred, typename Container>
void FlatSet<T, Pred, Container>::Reserve(SizeType new_capacity)
{
    internal_array.Reserve(new_capacity);
}

template <typename T, typename Pred, typename Container>
void FlatSet<T, Pred, Container>::ShrinkToFit()
{
    internal_array.ShrinkToFit();
}

template <typename T, typename Pred, typename Container>
void FlatSet<T, Pred, Container>::Swap(FlatSet& other) noexcept
{
    internal_array.Swap(other.internal_array);
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::IteratorType FlatSet<T, Pred, Container>::begin() noexcept
{
    return internal_array.begin();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::IteratorType FlatSet<T, Pred, Container>::end() noexcept
{
    return internal_array.end();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::ConstIteratorType FlatSet<T, Pred, Container>::begin() const noexcept
{
    return internal_array.begin();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::ConstIteratorType FlatSet<T, Pred, Container>::end() const noexcept
{
    return internal_array.end();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::ReverseIteratorType FlatSet<T, Pred, Container>::rbegin() noexcept
{
    return internal_array.rbegin();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::ReverseIteratorType FlatSet<T, Pred, Container>::rend() noexcept
{
    return internal_array.rend();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::ConstReverseIteratorType FlatSet<T, Pred, Container>::rbegin() const noexcept
{
    return internal_array.rbegin();
}

template <typename T, typename Pred, typename Container>
typename FlatSet<T, Pred, Container>::ConstReverseIteratorType FlatSet<T, Pred, Container>::rend() const noexcept
{
    return internal_array.rend();
}
} // namespace se
