#pragma once
#include <algorithm>


namespace se
{
template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::HashSet(SizeType capacity)
    : internal_set(capacity)
{
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::HashSet(std::initializer_list<ValueType> init_list)
    : internal_set(init_list)
{
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
HashSet<T, Hasher, KeyEq, Allocator>::HashSet(It first, Sent last)
    : internal_set(first, last)
{
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
HashSet<T, Hasher, KeyEq, Allocator> HashSet<T, Hasher, KeyEq, Allocator>::FromRange(Rng&& range)
{
    return HashSet{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::SizeType HashSet<T, Hasher, KeyEq, Allocator>::Len() const noexcept
{
    return internal_set.size();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
bool HashSet<T, Hasher, KeyEq, Allocator>::IsEmpty() const noexcept
{
    return internal_set.empty();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::SizeType HashSet<T, Hasher, KeyEq, Allocator>::Capacity() const noexcept
{
    return internal_set.bucket_count();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
void HashSet<T, Hasher, KeyEq, Allocator>::Reserve(SizeType new_capacity)
{
    internal_set.reserve(new_capacity);
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
void HashSet<T, Hasher, KeyEq, Allocator>::Clear() noexcept
{
    internal_set.clear();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
bool HashSet<T, Hasher, KeyEq, Allocator>::Insert(const ValueType& value)
{
    return Insert(ValueType{ value });
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
bool HashSet<T, Hasher, KeyEq, Allocator>::Insert(ValueType&& value)
{
    return internal_set.emplace(std::move(value)).second;
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
template <typename... Args>
bool HashSet<T, Hasher, KeyEq, Allocator>::Emplace(Args&&... args)
{
    return internal_set.emplace(std::forward<Args>(args)...).second;
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
bool HashSet<T, Hasher, KeyEq, Allocator>::Remove(const ValueType& value)
{
    return internal_set.erase(value) > 0;
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
HashSet<T, Hasher, KeyEq, Allocator>::SizeType HashSet<T, Hasher, KeyEq, Allocator>::RemoveIf(Predicate&& pred)
{
    return std::erase_if(internal_set, std::forward<Predicate>(pred));
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
bool HashSet<T, Hasher, KeyEq, Allocator>::Contains(const ValueType& value) const
{
    return internal_set.contains(value);
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
Array<typename HashSet<T, Hasher, KeyEq, Allocator>::ValueType> HashSet<T, Hasher, KeyEq, Allocator>::ToArray() const
{
    return Array<ValueType>{ internal_set.begin(), internal_set.end() };
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
void HashSet<T, Hasher, KeyEq, Allocator>::Swap(HashSet& other) noexcept
{
    std::swap(internal_set, other.internal_set);
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::Iterator HashSet<T, Hasher, KeyEq, Allocator>::begin() noexcept
{
    return internal_set.begin();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::Iterator HashSet<T, Hasher, KeyEq, Allocator>::end() noexcept
{
    return internal_set.end();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::ConstIterator HashSet<T, Hasher, KeyEq, Allocator>::begin() const noexcept
{
    return internal_set.begin();
}

template <typename T, typename Hasher, typename KeyEq, typename Allocator>
HashSet<T, Hasher, KeyEq, Allocator>::ConstIterator HashSet<T, Hasher, KeyEq, Allocator>::end() const noexcept
{
    return internal_set.end();
}
}
