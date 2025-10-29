#pragma once
#include <algorithm>


namespace se
{
template <typename T, typename Pred, typename Allocator>
Set<T, Pred, Allocator>::Set(std::initializer_list<ValueType> init_list)
    : internal_set(init_list)
{
}

template <typename T, typename Pred, typename Allocator>
template <std::input_iterator It>
    requires std::same_as<std::iter_value_t<It>, T>
Set<T, Pred, Allocator>::Set(It first, It last)
    : internal_set(first, last)
{
}

template <typename T, typename Pred, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
Set<T, Pred, Allocator> Set<T, Pred, Allocator>::FromRange(Rng&& range)
{
    return Set{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename T, typename Pred, typename Allocator>
Set<T, Pred, Allocator>::SizeType Set<T, Pred, Allocator>::Len() const noexcept
{
    return internal_set.size();
}

template <typename T, typename Pred, typename Allocator>
bool Set<T, Pred, Allocator>::IsEmpty() const noexcept
{
    return internal_set.empty();
}

template <typename T, typename Pred, typename Allocator>
void Set<T, Pred, Allocator>::Clear() noexcept
{
    internal_set.clear();
}

template <typename T, typename Pred, typename Allocator>
bool Set<T, Pred, Allocator>::Add(const ValueType& value)
{
    return Add(ValueType{ value });
}

template <typename T, typename Pred, typename Allocator>
bool Set<T, Pred, Allocator>::Add(ValueType&& value)
{
    return internal_set.emplace(std::move(value)).second;
}

template <typename T, typename Pred, typename Allocator>
template <typename... Args>
bool Set<T, Pred, Allocator>::Emplace(Args&&... args)
{
    return internal_set.emplace(std::forward<Args>(args)...).second;
}

template <typename T, typename Pred, typename Allocator>
bool Set<T, Pred, Allocator>::Remove(const ValueType& value)
{
    return internal_set.erase(value) > 0;
}

template <typename T, typename Pred, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
Set<T, Pred, Allocator>::SizeType Set<T, Pred, Allocator>::RemoveIf(Predicate&& pred)
{
    return std::erase_if(internal_set, std::forward<Predicate>(pred));
}

template <typename T, typename Pred, typename Allocator>
bool Set<T, Pred, Allocator>::Contains(const ValueType& value) const
{
    return internal_set.contains(value);
}

template <typename T, typename Pred, typename Allocator>
Array<typename Set<T, Pred, Allocator>::ValueType> Set<T, Pred, Allocator>::ToArray() const
{
    return Array<ValueType>{ internal_set.begin(), internal_set.end() };
}

template <typename T, typename Pred, typename Allocator>
Set<T, Pred, Allocator>::Iterator Set<T, Pred, Allocator>::begin() noexcept
{
    return internal_set.begin();
}

template <typename T, typename Pred, typename Allocator>
Set<T, Pred, Allocator>::Iterator Set<T, Pred, Allocator>::end() noexcept
{
    return internal_set.end();
}

template <typename T, typename Pred, typename Allocator>
Set<T, Pred, Allocator>::ConstIterator Set<T, Pred, Allocator>::begin() const noexcept
{
    return internal_set.begin();
}

template <typename T, typename Pred, typename Allocator>
Set<T, Pred, Allocator>::ConstIterator Set<T, Pred, Allocator>::end() const noexcept
{
    return internal_set.end();
}
}
