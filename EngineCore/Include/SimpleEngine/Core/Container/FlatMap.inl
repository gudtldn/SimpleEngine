// ReSharper disable CppRedundantTypenameKeyword
#pragma once

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <utility>

#include "SimpleEngine/Utility/Debug.h"


namespace se
{
template <typename Key, typename Value, typename Pred, typename Allocator>
FlatMap<Key, Value, Pred, Allocator>::FlatMap(std::initializer_list<PairType> init_list)
{
    internal_array.Reserve(init_list.size());
    for (const PairType& p : init_list)
    {
        internal_array.Push(MutablePairType{ p.first, p.second });
    }

    std::sort(internal_array.begin(), internal_array.end(), pair_compare);
    // 중복 키 제거
    auto unique_end = std::unique(internal_array.begin(), internal_array.end(), [this](const MutablePairType& lhs, const MutablePairType& rhs)
    {
        return !pair_compare.compare(lhs.first, rhs.first) && !pair_compare.compare(rhs.first, lhs.first);
    });
    internal_array.Truncate(std::distance(internal_array.begin(), unique_end));
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
FlatMap<Key, Value, Pred, Allocator>::FlatMap(It first, Sent last)
    : internal_array(first, last)
{
    std::sort(internal_array.begin(), internal_array.end(), pair_compare);
    auto unique_end = std::unique(internal_array.begin(), internal_array.end(), [this](const MutablePairType& lhs, const MutablePairType& rhs)
    {
        return !pair_compare.compare(lhs.first, rhs.first) && !pair_compare.compare(rhs.first, lhs.first);
    });
    internal_array.Truncate(std::distance(internal_array.begin(), unique_end));
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <std::ranges::input_range Rng>
FlatMap<Key, Value, Pred, Allocator> FlatMap<Key, Value, Pred, Allocator>::FromRange(Rng&& range)
{
    return FlatMap(std::ranges::begin(range), std::ranges::end(range));
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::SizeType FlatMap<Key, Value, Pred, Allocator>::Len() const noexcept
{
    return internal_array.Len();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
bool FlatMap<Key, Value, Pred, Allocator>::IsEmpty() const noexcept
{
    return internal_array.IsEmpty();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::SizeType FlatMap<Key, Value, Pred, Allocator>::Capacity() const noexcept
{
    return internal_array.Capacity();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
void FlatMap<Key, Value, Pred, Allocator>::Clear() noexcept
{
    internal_array.Clear();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename V>
    requires std::constructible_from<Value, V&&>
typename FlatMap<Key, Value, Pred, Allocator>::ValueType& FlatMap<Key, Value, Pred, Allocator>::Insert(const KeyType& key, V&& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        it->second = std::forward<V>(value);
        return it->second;
    }

    auto index = std::distance(internal_array.begin(), it);
    internal_array.Insert(index, std::make_pair(key, std::forward<V>(value)));
    return internal_array[index].second;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename V>
    requires std::constructible_from<Value, V&&>
typename FlatMap<Key, Value, Pred, Allocator>::ValueType& FlatMap<Key, Value, Pred, Allocator>::Insert(KeyType&& key, V&& value)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        it->second = std::forward<V>(value);
        return it->second;
    }

    auto index = std::distance(internal_array.begin(), it);
    internal_array.Insert(index, std::make_pair(std::move(key), std::forward<V>(value)));
    return internal_array[index].second;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename ... Args>
    requires std::constructible_from<Value, Args&&...>
typename FlatMap<Key, Value, Pred, Allocator>::ValueType& FlatMap<Key, Value, Pred, Allocator>::Emplace(const KeyType& key, Args&&... args)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        it->second = Value(std::forward<Args>(args)...);
        return it->second;
    }

    auto index = std::distance(internal_array.begin(), it);
    internal_array.Insert(index, std::make_pair(key, Value(std::forward<Args>(args)...)));
    return internal_array[index].second;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename ... Args>
    requires std::constructible_from<Value, Args&&...>
typename FlatMap<Key, Value, Pred, Allocator>::ValueType& FlatMap<Key, Value, Pred, Allocator>::Emplace(KeyType&& key, Args&&... args)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        it->second = Value(std::forward<Args>(args)...);
        return it->second;
    }

    auto index = std::distance(internal_array.begin(), it);
    internal_array.Insert(index, std::make_pair(std::move(key), Value(std::forward<Args>(args)...)));
    return internal_array[index].second;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::EntryType FlatMap<Key, Value, Pred, Allocator>::Entry(const KeyType& key)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        return EntryType(typename EntryType::OccupiedEntry(it, this));
    }
    return EntryType(typename EntryType::VacantEntry(key, this));
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::EntryType FlatMap<Key, Value, Pred, Allocator>::Entry(KeyType&& key)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        return EntryType(typename EntryType::OccupiedEntry(it, this));
    }
    return EntryType(typename EntryType::VacantEntry(std::move(key), this));
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::ValueType&> FlatMap<Key, Value, Pred, Allocator>::Find(const KeyType& key)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        return it->second;
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename FlatMap<Key, Value, Pred, Allocator>::ValueType&> FlatMap<Key, Value, Pred, Allocator>::Find(const KeyType& key) const
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        return it->second;
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ValueType& FlatMap<Key, Value, Pred, Allocator>::FindChecked(const KeyType& key)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    SE_ASSERT_RELEASE(it != internal_array.end() && !pair_compare.compare(key, it->first));
    return it->second;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
const typename FlatMap<Key, Value, Pred, Allocator>::ValueType& FlatMap<Key, Value, Pred, Allocator>::FindChecked(const KeyType& key) const
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    SE_ASSERT_RELEASE(it != internal_array.end() && !pair_compare.compare(key, it->first));
    return it->second;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::FindBy(Predicate&& pred)
{
    for (auto& pair : internal_array)
    {
        if (pred(pair.first, pair.second))
        {
            return AsPairType(pair);
        }
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
Optional<const typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::FindBy(Predicate&& pred) const
{
    for (const auto& pair : internal_array)
    {
        if (pred(pair.first, pair.second))
        {
            return reinterpret_cast<const PairType&>(pair);
        }
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::Front() noexcept
{
    return internal_array.Front().Map([](MutablePairType& pair) -> PairType&
    {
        return AsPairType(pair);
    });
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::Front() const noexcept
{
    return internal_array.Front().Map([](const MutablePairType& pair) -> const PairType&
    {
        return reinterpret_cast<const PairType&>(pair);
    });
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::Back() noexcept
{
    return internal_array.Back().Map([](MutablePairType& pair) -> PairType&
    {
        return AsPairType(pair);
    });
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::Back() const noexcept
{
    return internal_array.Back().Map([](const MutablePairType& pair) -> const PairType&
    {
        return reinterpret_cast<const PairType&>(pair);
    });
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType> FlatMap<Key, Value, Pred, Allocator>::PopFront()
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    PairType pair = std::move(internal_array[0]);
    internal_array.RemoveAt(0);
    return pair;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType> FlatMap<Key, Value, Pred, Allocator>::PopBack()
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    return internal_array.Pop();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::LowerBoundEntry(const KeyType& key)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end())
    {
        return AsPairType(*it);
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::LowerBoundEntry(const KeyType& key) const
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end())
    {
        return reinterpret_cast<const PairType&>(*it);
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::UpperBoundEntry(const KeyType& key)
{
    auto it = std::upper_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end())
    {
        return AsPairType(*it);
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Optional<const typename FlatMap<Key, Value, Pred, Allocator>::PairType&> FlatMap<Key, Value, Pred, Allocator>::UpperBoundEntry(const KeyType& key) const
{
    auto it = std::upper_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end())
    {
        return reinterpret_cast<const PairType&>(*it);
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
bool FlatMap<Key, Value, Pred, Allocator>::Contains(const KeyType& key) const
{
    return std::binary_search(internal_array.begin(), internal_array.end(), key, pair_compare);
}

template <typename Key, typename Value, typename Pred, typename Allocator>
bool FlatMap<Key, Value, Pred, Allocator>::Remove(const KeyType& key)
{
    auto it = std::lower_bound(internal_array.begin(), internal_array.end(), key, pair_compare);
    if (it != internal_array.end() && !pair_compare.compare(key, it->first))
    {
        internal_array.RemoveAt(std::distance(internal_array.begin(), it));
        return true;
    }
    return false;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
typename FlatMap<Key, Value, Pred, Allocator>::SizeType FlatMap<Key, Value, Pred, Allocator>::RemoveIf(Predicate&& pred)
{
    return internal_array.RemoveIf([&pred](const MutablePairType& pair)
    {
        return std::forward<Predicate>(pred)(pair.first, pair.second);
    });
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename T>
    requires std::constructible_from<T, const Key&>
Array<T> FlatMap<Key, Value, Pred, Allocator>::Keys() const
{
    Array<T> keys;
    keys.Reserve(Len());
    for (const auto& pair : internal_array)
    {
        keys.Emplace(pair.first);
    }
    return keys;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
template <typename T>
    requires std::constructible_from<T, const Value&>
Array<T> FlatMap<Key, Value, Pred, Allocator>::Values() const
{
    Array<T> values;
    values.Reserve(Len());
    for (const auto& pair : internal_array)
    {
        values.Emplace(pair.second);
    }
    return values;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
Array<typename FlatMap<Key, Value, Pred, Allocator>::PairType> FlatMap<Key, Value, Pred, Allocator>::ToArray() const
{
    Array<PairType> result;
    result.Reserve(Len());
    for (const auto& pair : internal_array)
    {
        result.Push(pair);
    }
    return result;
}

template <typename Key, typename Value, typename Pred, typename Allocator>
void FlatMap<Key, Value, Pred, Allocator>::Reserve(SizeType new_capacity)
{
    internal_array.Reserve(new_capacity);
}

template <typename Key, typename Value, typename Pred, typename Allocator>
void FlatMap<Key, Value, Pred, Allocator>::ShrinkToFit()
{
    internal_array.ShrinkToFit();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
void FlatMap<Key, Value, Pred, Allocator>::Swap(FlatMap& other) noexcept
{
    internal_array.Swap(other.internal_array);
}


template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::IteratorType FlatMap<Key, Value, Pred, Allocator>::begin() noexcept
{
    return internal_array.begin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::IteratorType FlatMap<Key, Value, Pred, Allocator>::end() noexcept
{
    return internal_array.end();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ConstIteratorType FlatMap<Key, Value, Pred, Allocator>::begin() const noexcept
{
    return internal_array.begin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ConstIteratorType FlatMap<Key, Value, Pred, Allocator>::end() const noexcept
{
    return internal_array.end();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ReverseIteratorType FlatMap<Key, Value, Pred, Allocator>::rbegin() noexcept
{
    return internal_array.rbegin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ReverseIteratorType FlatMap<Key, Value, Pred, Allocator>::rend() noexcept
{
    return internal_array.rend();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ConstReverseIteratorType FlatMap<Key, Value, Pred, Allocator>::rbegin() const noexcept
{
    return internal_array.rbegin();
}

template <typename Key, typename Value, typename Pred, typename Allocator>
typename FlatMap<Key, Value, Pred, Allocator>::ConstReverseIteratorType FlatMap<Key, Value, Pred, Allocator>::rend() const noexcept
{
    return internal_array.rend();
}
} // namespace se
