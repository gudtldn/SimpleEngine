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
template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FlatMap(std::initializer_list<PairType> init_list)
{
    keys.Reserve(init_list.size());
    values.Reserve(init_list.size());
    for (const PairType& p : init_list)
    {
        keys.Push(p.first);
        values.Push(p.second);
    }
    SortAndDeduplicate();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <std::input_iterator It, std::sentinel_for<It> Sent>
FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FlatMap(It first, Sent last)
{
    for (auto it = first; it != last; ++it)
    {
        auto&& pair = *it;
        keys.Push(pair.first);
        values.Push(pair.second);
    }
    SortAndDeduplicate();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <std::ranges::input_range Rng>
FlatMap<Key, Value, Pred, KeyContainer, ValueContainer> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FromRange(Rng&& range)
{
    return FlatMap(std::ranges::begin(range), std::ranges::end(range));
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::SizeType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Len() const noexcept
{
    return keys.Len();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
bool FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::IsEmpty() const noexcept
{
    return keys.IsEmpty();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::SizeType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Capacity() const noexcept
{
    return keys.Capacity();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
void FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Clear() noexcept
{
    keys.Clear();
    values.Clear();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename V>
    requires std::constructible_from<Value, V&&>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType& FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Insert(const KeyType& key, V&& value)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
    if (it != keys.end() && !compare(key, *it))
    {
        values[index] = std::forward<V>(value);
        return values[index];
    }

    keys.Insert(index, key);
    try
    {
        values.Insert(index, std::forward<V>(value));
    }
    catch (...)
    {
        keys.RemoveAt(index);
        throw;
    }
    return values[index];
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename V>
    requires std::constructible_from<Value, V&&>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType& FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Insert(KeyType&& key, V&& value)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
    if (it != keys.end() && !compare(key, *it))
    {
        values[index] = std::forward<V>(value);
        return values[index];
    }

    keys.Insert(index, std::move(key));
    try
    {
        values.Insert(index, std::forward<V>(value));
    }
    catch (...)
    {
        keys.RemoveAt(index);
        throw;
    }
    return values[index];
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename... Args>
    requires std::constructible_from<Value, Args&&...>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType& FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Emplace(const KeyType& key, Args&&... args)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
    if (it != keys.end() && !compare(key, *it))
    {
        values[index] = Value(std::forward<Args>(args)...);
        return values[index];
    }

    keys.Insert(index, key);
    try
    {
        values.Insert(index, Value(std::forward<Args>(args)...));
    }
    catch (...)
    {
        keys.RemoveAt(index);
        throw;
    }
    return values[index];
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename... Args>
    requires std::constructible_from<Value, Args&&...>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType& FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Emplace(KeyType&& key, Args&&... args)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
    if (it != keys.end() && !compare(key, *it))
    {
        values[index] = Value(std::forward<Args>(args)...);
        return values[index];
    }

    keys.Insert(index, std::move(key));
    try
    {
        values.Insert(index, Value(std::forward<Args>(args)...));
    }
    catch (...)
    {
        keys.RemoveAt(index);
        throw;
    }
    return values[index];
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::EntryType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Entry(const KeyType& key)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end() && !compare(key, *it))
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        return EntryType(typename EntryType::OccupiedEntry(index, this));
    }
    return EntryType(typename EntryType::VacantEntry(key, this));
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::EntryType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Entry(KeyType&& key)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end() && !compare(key, *it))
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        return EntryType(typename EntryType::OccupiedEntry(index, this));
    }
    return EntryType(typename EntryType::VacantEntry(std::move(key), this));
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType&> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Find(const KeyType& key)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end() && !compare(key, *it))
    {
        return values[std::distance(keys.begin(), it)];
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<const typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType&> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Find(const KeyType& key) const
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end() && !compare(key, *it))
    {
        return values[std::distance(keys.begin(), it)];
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType& FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FindChecked(const KeyType& key)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    SE_ASSERT_RELEASE(it != keys.end() && !compare(key, *it));
    return values[std::distance(keys.begin(), it)];
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
const typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ValueType& FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FindChecked(const KeyType& key) const
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    SE_ASSERT_RELEASE(it != keys.end() && !compare(key, *it));
    return values[std::distance(keys.begin(), it)];
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FindBy(Predicate&& pred)
{
    for (SizeType i = 0; i < keys.Len(); ++i)
    {
        if (pred(keys[i], values[i]))
        {
            return ReferenceType{ keys[i], values[i] };
        }
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::FindBy(Predicate&& pred) const
{
    for (SizeType i = 0; i < keys.Len(); ++i)
    {
        if (pred(keys[i], values[i]))
        {
            return ConstReferenceType{ keys[i], values[i] };
        }
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Front() noexcept
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    return ReferenceType{ keys[0], values[0] };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Front() const noexcept
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    return ConstReferenceType{ keys[0], values[0] };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Back() noexcept
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    auto last = keys.Len() - 1;
    return ReferenceType{ keys[last], values[last] };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Back() const noexcept
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    auto last = keys.Len() - 1;
    return ConstReferenceType{ keys[last], values[last] };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::PairType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::PopFront()
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    PairType pair{ std::move(keys[0]), std::move(values[0]) };
    keys.RemoveAt(0);
    values.RemoveAt(0);
    return pair;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::PairType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::PopBack()
{
    if (IsEmpty())
    {
        return NullOpt;
    }
    auto last = keys.Len() - 1;
    PairType pair{ std::move(keys[last]), std::move(values[last]) };
    keys.RemoveAt(last);
    values.RemoveAt(last);
    return pair;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::LowerBoundEntry(const KeyType& key)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end())
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        return ReferenceType{ keys[index], values[index] };
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::LowerBoundEntry(const KeyType& key) const
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end())
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        return ConstReferenceType{ keys[index], values[index] };
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::UpperBoundEntry(const KeyType& key)
{
    auto it = std::upper_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end())
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        return ReferenceType{ keys[index], values[index] };
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Optional<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReferenceType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::UpperBoundEntry(const KeyType& key) const
{
    auto it = std::upper_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end())
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        return ConstReferenceType{ keys[index], values[index] };
    }
    return NullOpt;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
bool FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Contains(const KeyType& key) const
{
    return std::binary_search(keys.begin(), keys.end(), key, compare);
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
bool FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Remove(const KeyType& key)
{
    auto it = std::lower_bound(keys.begin(), keys.end(), key, compare);
    if (it != keys.end() && !compare(key, *it))
    {
        auto index = static_cast<SizeType>(std::distance(keys.begin(), it));
        keys.RemoveAt(index);
        values.RemoveAt(index);
        return true;
    }
    return false;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename Predicate>
    requires std::predicate<Predicate, const Key&, const Value&>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::SizeType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::RemoveIf(Predicate&& pred)
{
    SizeType write = 0;
    for (SizeType read = 0; read < keys.Len(); ++read)
    {
        if (!pred(keys[read], values[read]))
        {
            if (write != read)
            {
                keys[write] = std::move(keys[read]);
                values[write] = std::move(values[read]);
            }
            ++write;
        }
    }

    SizeType removed = keys.Len() - write;
    keys.Truncate(write);
    values.Truncate(write);
    return removed;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename T>
    requires std::constructible_from<T, const Key&>
Array<T> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Keys() const
{
    Array<T> result;
    result.Reserve(Len());
    for (const auto& key : keys)
    {
        result.Emplace(key);
    }
    return result;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
template <typename T>
    requires std::constructible_from<T, const Value&>
Array<T> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Values() const
{
    Array<T> result;
    result.Reserve(Len());
    for (const auto& value : values)
    {
        result.Emplace(value);
    }
    return result;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
Array<typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::PairType> FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ToArray() const
{
    Array<PairType> result;
    result.Reserve(Len());
    for (SizeType i = 0; i < keys.Len(); ++i)
    {
        result.Push(PairType{ keys[i], values[i] });
    }
    return result;
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
void FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Reserve(SizeType new_capacity)
{
    keys.Reserve(new_capacity);
    values.Reserve(new_capacity);
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
void FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ShrinkToFit()
{
    keys.ShrinkToFit();
    values.ShrinkToFit();
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
void FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::Swap(FlatMap& other) noexcept
{
    keys.Swap(other.keys);
    values.Swap(other.values);
}


template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::IteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::begin() noexcept
{
    return IteratorType{ keys.Data(), values.Data() };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::IteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::end() noexcept
{
    return IteratorType{ keys.Data() + keys.Len(), values.Data() + values.Len() };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstIteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::begin() const noexcept
{
    return ConstIteratorType{ keys.Data(), values.Data() };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstIteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::end() const noexcept
{
    return ConstIteratorType{ keys.Data() + keys.Len(), values.Data() + values.Len() };
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReverseIteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::rbegin() noexcept
{
    return std::make_reverse_iterator(end());
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ReverseIteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::rend() noexcept
{
    return std::make_reverse_iterator(begin());
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReverseIteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::rbegin() const noexcept
{
    return std::make_reverse_iterator(end());
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
typename FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::ConstReverseIteratorType FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::rend() const noexcept
{
    return std::make_reverse_iterator(begin());
}

template <typename Key, typename Value, typename Pred, typename KeyContainer, typename ValueContainer>
void FlatMap<Key, Value, Pred, KeyContainer, ValueContainer>::SortAndDeduplicate()
{
    MutableIteratorType mbegin{ keys.Data(), values.Data() };
    MutableIteratorType mend{ keys.Data() + keys.Len(), values.Data() + values.Len() };

    std::sort(mbegin, mend, [this](const auto& lhs, const auto& rhs)
    {
        return compare(lhs.first, rhs.first);
    });

    auto unique_end = std::unique(mbegin, mend, [this](const auto& lhs, const auto& rhs)
    {
        return !compare(lhs.first, rhs.first) && !compare(rhs.first, lhs.first);
    });

    auto new_size = static_cast<SizeType>(std::distance(mbegin, unique_end));
    keys.Truncate(new_size);
    values.Truncate(new_size);
}
} // namespace se
