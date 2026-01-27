#pragma once
#include <cassert>


namespace se
{
template <typename T, typename Allocator>
Deque<T, Allocator>::Deque() noexcept(noexcept(Allocator()))
    : internal_deque()
{
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(SizeType count)
    : internal_deque(count)
{
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(SizeType count, const ValueType& value)
    : internal_deque(count, value)
{
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(std::initializer_list<ValueType> init_list)
    : internal_deque(init_list)
{
}

template <typename T, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
Deque<T, Allocator>::Deque(It first, Sent last)
    : internal_deque(first, last)
{
}

template <typename T, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
Deque<T, Allocator> Deque<T, Allocator>::FromRange(Rng&& range)
{
    return Deque{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename T, typename Allocator>
Deque<T, Allocator>::SizeType Deque<T, Allocator>::Len() const noexcept
{
    return internal_deque.size();
}

template <typename T, typename Allocator>
bool Deque<T, Allocator>::IsEmpty() const noexcept
{
    return internal_deque.empty();
}

/** Deque의 크기를 변경합니다. (기본 생성자로 채움) */
template <typename T, typename Allocator>
void Deque<T, Allocator>::Resize(SizeType count)
{
    internal_deque.resize(count);
}

/** Deque의 크기를 변경합니다. (주어진 값으로 채움) */
template <typename T, typename Allocator>
void Deque<T, Allocator>::Resize(SizeType count, const ValueType& value)
{
    internal_deque.resize(count, value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::ShrinkToFit()
{
    internal_deque.shrink_to_fit();
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::Clear() noexcept
{
    internal_deque.clear();
}

template <typename T, typename Allocator>
Optional<T&> Deque<T, Allocator>::At(SizeType index)
{
    if (index >= internal_deque.size())
    {
        return std::nullopt;
    }
    return internal_deque[index];
}

template <typename T, typename Allocator>
Optional<const T&> Deque<T, Allocator>::At(SizeType index) const
{
    if (index >= internal_deque.size())
    {
        return std::nullopt;
    }
    return internal_deque[index];
}

template <typename T, typename Allocator>
Optional<T&> Deque<T, Allocator>::Front()
{
    return At(0);
}

template <typename T, typename Allocator>
Optional<const T&> Deque<T, Allocator>::Front() const
{
    return At(0);
}

template <typename T, typename Allocator>
Optional<T&> Deque<T, Allocator>::Back()
{
    return At(internal_deque.size() - 1);
}

template <typename T, typename Allocator>
Optional<const T&> Deque<T, Allocator>::Back() const
{
    return At(internal_deque.size() - 1);
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::PushFront(const ValueType& value)
{
    return PushFront(ValueType{ value });
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::PushFront(ValueType&& value)
{
    internal_deque.push_front(std::move(value));
    return internal_deque.front();
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::PushBack(const ValueType& value)
{
    return PushBack(ValueType{ value });
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::PushBack(ValueType&& value)
{
    internal_deque.push_back(std::move(value));
    return internal_deque.back();
}

template <typename T, typename Allocator>
template <typename... Args>
T& Deque<T, Allocator>::EmplaceFront(Args&&... args)
{
    return internal_deque.emplace_front(std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
template <typename... Args>
T& Deque<T, Allocator>::EmplaceBack(Args&&... args)
{
    return internal_deque.emplace_back(std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
Optional<typename Deque<T, Allocator>::ValueType> Deque<T, Allocator>::PopFront()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    ValueType value = std::move(internal_deque.front());
    internal_deque.pop_front();
    return value;
}

template <typename T, typename Allocator>
Optional<typename Deque<T, Allocator>::ValueType> Deque<T, Allocator>::PopBack()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    ValueType value = std::move(internal_deque.back());
    internal_deque.pop_back();
    return value;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::Insert(SizeType index, const ValueType& value)
{
    Insert(index, ValueType{ value });
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::Insert(SizeType index, ValueType&& value)
{
    assert(index <= internal_deque.size() && "Insert index out of bounds");

    internal_deque.insert(internal_deque.begin() + index, std::move(value));
}

template <typename T, typename Allocator>
template <std::input_iterator It>
    requires std::same_as<std::iter_value_t<It>, T>
void Deque<T, Allocator>::Insert(SizeType index, It first, It last)
{
    assert(index <= internal_deque.size() && "Insert index out of bounds");

    internal_deque.insert(internal_deque.begin() + index, first, last);
}

template <typename T, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
void Deque<T, Allocator>::InsertRange(SizeType index, Rng&& range)
{
    Insert(index, std::ranges::begin(range), std::ranges::end(range));
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::RemoveAt(SizeType index)
{
    assert(index < internal_deque.size() && "RemoveAt index out of bounds");

    internal_deque.erase(internal_deque.begin() + index);
}

template <typename T, typename Allocator>
Deque<T, Allocator>::SizeType Deque<T, Allocator>::Remove(const ValueType& value)
{
    return std::erase(internal_deque, value);
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
Deque<T, Allocator>::SizeType Deque<T, Allocator>::RemoveIf(Predicate&& pred)
{
    return std::erase_if(internal_deque, std::forward<Predicate>(pred));
}

template <typename T, typename Allocator>
bool Deque<T, Allocator>::Contains(const ValueType& value) const
{
    return std::find(begin(), end(), value) != end();
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::Swap(Deque& other) noexcept
{
    std::swap(internal_deque, other.internal_deque);
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::operator[](SizeType index) noexcept
{
    return internal_deque[index];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::operator[](SizeType index) const noexcept
{
    return internal_deque[index];
}

template <typename T, typename Allocator>
Deque<T, Allocator>::IteratorType Deque<T, Allocator>::begin() noexcept
{
    return internal_deque.begin();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::IteratorType Deque<T, Allocator>::end() noexcept
{
    return internal_deque.end();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::ConstIteratorType Deque<T, Allocator>::begin() const noexcept
{
    return internal_deque.cbegin();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::ConstIteratorType Deque<T, Allocator>::end() const noexcept
{
    return internal_deque.cend();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::ReverseIteratorType Deque<T, Allocator>::rbegin() noexcept
{
    return internal_deque.rbegin();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::ReverseIteratorType Deque<T, Allocator>::rend() noexcept
{
    return internal_deque.rend();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::ConstReverseIteratorType Deque<T, Allocator>::rbegin() const noexcept
{
    return internal_deque.crbegin();
}

template <typename T, typename Allocator>
Deque<T, Allocator>::ConstReverseIteratorType Deque<T, Allocator>::rend() const noexcept
{
    return internal_deque.crend();
}
}  // namespace se
