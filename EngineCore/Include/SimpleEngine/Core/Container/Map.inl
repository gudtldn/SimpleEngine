// ReSharper disable CppRedundantTypenameKeyword
#pragma once


namespace se
{
template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::Map(std::initializer_list<PairType> init_list)
    : internal_map(init_list)
{
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <std::input_iterator It>
Map<Key, Value, Pred, Allocator>::Map(It first, It last)
    : internal_map(first, last)
{
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <std::ranges::input_range Rng>
Map<Key, Value, Pred, Allocator> Map<Key, Value, Pred, Allocator>::FromRange(Rng&& range)
{
    return Map{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::SizeType Map<Key, Value, Pred, Allocator>::Len() const noexcept
{
    return internal_map.size();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
bool Map<Key, Value, Pred, Allocator>::IsEmpty() const noexcept
{
    return internal_map.empty();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
void Map<Key, Value, Pred, Allocator>::Clear() noexcept
{
    internal_map.clear();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename... Args>
typename Map<Key, Value, Pred, Allocator>::ValueType& Map<Key, Value, Pred, Allocator>::Emplace(const KeyType& key, Args&&... args)
{
    internal_map.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(std::forward<Args>(args)...)
    );
    return internal_map.at(key);
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::EntryType Map<Key, Value, Pred, Allocator>::Entry(const KeyType& key)
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return EntryType(typename EntryType::OccupiedEntry(it, this));
    }
    return EntryType(typename EntryType::VacantEntry(key, this));
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename Map<Key, Value, Pred, Allocator>::ValueType&> Map<Key, Value, Pred, Allocator>::Find(const KeyType& key)
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return it->second;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename Map<Key, Value, Pred, Allocator>::ValueType&> Map<Key, Value, Pred, Allocator>::Find(const KeyType& key) const
{
    if (auto it = internal_map.find(key); it != internal_map.end())
    {
        return it->second;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::First() noexcept
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return *internal_map.begin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::First() const noexcept
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return *internal_map.begin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::Last() noexcept
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return *(--internal_map.end());
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::Last() const noexcept
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return *(--internal_map.end());
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::LowerBoundEntry(const KeyType& key)
{
    if (auto it = internal_map.lower_bound(key); it != internal_map.end())
    {
        return *it;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::LowerBoundEntry(const KeyType& key) const
{
    if (auto it = internal_map.lower_bound(key); it != internal_map.end())
    {
        return *it;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::UpperBoundEntry(const KeyType& key)
{
    if (auto it = internal_map.upper_bound(key); it != internal_map.end())
    {
        return *it;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename Map<Key, Value, Pred, Allocator>::PairType&> Map<Key, Value, Pred, Allocator>::UpperBoundEntry(const KeyType& key) const
{
    if (auto it = internal_map.upper_bound(key); it != internal_map.end())
    {
        return *it;
    }
    return std::nullopt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
bool Map<Key, Value, Pred, Allocator>::Contains(const KeyType& key) const
{
    return internal_map.contains(key);
}

template <typename Key, typename Value, typename Pred, typename Allocator>
bool Map<Key, Value, Pred, Allocator>::Remove(const KeyType& key)
{
    return internal_map.erase(key) > 0;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
Map<Key, Value, Pred, Allocator>::SizeType Map<Key, Value, Pred, Allocator>::RemoveIf(Predicate&& pred)
{
    return std::erase_if(internal_map, [&pred](const PairType& pair) -> bool
    {
        return std::forward<Predicate>(pred)(pair.first, pair.second);
    });
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Array<typename Map<Key, Value, Pred, Allocator>::KeyType> Map<Key, Value, Pred, Allocator>::GetKeys() const
{
    Array<KeyType> keys;

    keys.Reserve(Len());
    for (const auto& pair : internal_map)
    {
        keys.Push(pair.first);
    }
    return keys;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Array<typename Map<Key, Value, Pred, Allocator>::ValueType> Map<Key, Value, Pred, Allocator>::GetValues() const
{
    Array<ValueType> values;

    values.Reserve(Len());
    for (const auto& pair : internal_map)
    {
        values.Push(pair.second);
    }
    return values;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
void Map<Key, Value, Pred, Allocator>::Swap(Map& other) noexcept
{
    std::swap(internal_map, other.internal_map);
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::ValueType& Map<Key, Value, Pred, Allocator>::operator[](const KeyType& key)
{
    return internal_map[key];
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::IteratorType Map<Key, Value, Pred, Allocator>::begin() noexcept
{
    return internal_map.begin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::IteratorType Map<Key, Value, Pred, Allocator>::end() noexcept
{
    return internal_map.end();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::ConstIteratorType Map<Key, Value, Pred, Allocator>::begin() const noexcept
{
    return internal_map.begin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Map<Key, Value, Pred, Allocator>::ConstIteratorType Map<Key, Value, Pred, Allocator>::end() const noexcept
{
    return internal_map.end();
}
}
