// ReSharper disable CppRedundantTypenameKeyword
#pragma once
#include <utility>


namespace se
{
template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::HashMap(SizeType capacity)
    : internal_map(capacity)
{
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::HashMap(std::initializer_list<PairType> init_list)
    : internal_map(init_list)
{
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::HashMap(It first, Sent last)
    : internal_map(first, last)
{
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
template <std::ranges::input_range Rng>
HashMap<Key, Value, Hasher, KeyEq, Allocator> HashMap<Key, Value, Hasher, KeyEq, Allocator>::FromRange(Rng&& range)
{
    return HashMap{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::SizeType HashMap<Key, Value, Hasher, KeyEq, Allocator>::Len() const noexcept
{
    return internal_map.size();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
bool HashMap<Key, Value, Hasher, KeyEq, Allocator>::IsEmpty() const noexcept
{
    return internal_map.empty();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::SizeType HashMap<Key, Value, Hasher, KeyEq, Allocator>::Capacity() const noexcept
{
    return internal_map.bucket_count();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
void HashMap<Key, Value, Hasher, KeyEq, Allocator>::Reserve(SizeType new_capacity)
{
    internal_map.reserve(new_capacity);
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
void HashMap<Key, Value, Hasher, KeyEq, Allocator>::Clear() noexcept
{
    internal_map.clear();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType& HashMap<Key, Value, Hasher, KeyEq, Allocator>::Insert(
    const KeyType& key, const ValueType& value
)
{
    return Insert(key, ValueType{ value });
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType& HashMap<Key, Value, Hasher, KeyEq, Allocator>::Insert(
    const KeyType& key, ValueType&& value
)
{
    internal_map.insert_or_assign(key, std::move(value));
    return internal_map.at(key);
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
template <typename... Args>
typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType& HashMap<Key, Value, Hasher, KeyEq, Allocator>::Emplace(
    const KeyType& key, Args&&... args
)
{
    internal_map.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(std::forward<Args>(args)...)
    );
    return internal_map.at(key);
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::EntryType HashMap<Key, Value, Hasher, KeyEq, Allocator>::Entry(const KeyType& key)
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return EntryType(typename EntryType::OccupiedEntry(it, this));
    }
    return EntryType(typename EntryType::VacantEntry(key, this));
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
Optional<typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType&> HashMap<Key, Value, Hasher, KeyEq, Allocator>::Find(const KeyType& key)
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return it->second;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
Optional<const typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType&> HashMap<Key, Value, Hasher, KeyEq, Allocator>::Find(
    const KeyType& key
) const
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return it->second;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType& HashMap<Key, Value, Hasher, KeyEq, Allocator>::FindChecked(const KeyType& key)
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return it->second;
    }
    std::unreachable();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
const typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType& HashMap<Key, Value, Hasher, KeyEq, Allocator>::FindChecked(
    const KeyType& key
) const
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return it->second;
    }
    std::unreachable();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
bool HashMap<Key, Value, Hasher, KeyEq, Allocator>::Contains(const KeyType& key) const
{
    return internal_map.contains(key);
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
bool HashMap<Key, Value, Hasher, KeyEq, Allocator>::Remove(const KeyType& key)
{
    return internal_map.erase(key) > 0;
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::SizeType HashMap<Key, Value, Hasher, KeyEq, Allocator>::RemoveIf(Predicate&& pred)
{
    return std::erase_if(internal_map, [&pred](const PairType& pair) -> bool
    {
        return std::forward<Predicate>(pred)(pair.first, pair.second);
    });
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
Array<typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::KeyType> HashMap<Key, Value, Hasher, KeyEq, Allocator>::Keys() const
{
    Array<KeyType> keys;

    keys.Reserve(internal_map.size());
    for (const auto& pair : internal_map)
    {
        keys.Push(pair.first);
    }
    return keys;
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
Array<typename HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType> HashMap<Key, Value, Hasher, KeyEq, Allocator>::Values() const
{
    Array<ValueType> values;

    values.Reserve(internal_map.size());
    for (const auto& pair : internal_map)
    {
        values.Push(pair.second);
    }
    return values;
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
void HashMap<Key, Value, Hasher, KeyEq, Allocator>::Swap(HashMap& other) noexcept
{
    std::swap(internal_map, other.internal_map);
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::ValueType& HashMap<Key, Value, Hasher, KeyEq, Allocator>::operator[](const KeyType& key)
{
    return internal_map[key];
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::IteratorType HashMap<Key, Value, Hasher, KeyEq, Allocator>::begin() noexcept
{
    return internal_map.begin();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::IteratorType HashMap<Key, Value, Hasher, KeyEq, Allocator>::end() noexcept
{
    return internal_map.end();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::ConstIteratorType HashMap<Key, Value, Hasher, KeyEq, Allocator>::begin() const noexcept
{
    return internal_map.begin();
}

template <typename Key, typename Value, typename Hasher, typename KeyEq, typename Allocator>
HashMap<Key, Value, Hasher, KeyEq, Allocator>::ConstIteratorType HashMap<Key, Value, Hasher, KeyEq, Allocator>::end() const noexcept
{
    return internal_map.end();
}
}
