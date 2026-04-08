#pragma once
#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>

#include "SimpleEngine/Utility/Debug.h"


namespace se
{
template <typename T, typename Allocator>
Array<T, Allocator>::Array() noexcept(noexcept(Allocator()))
    : data(nullptr)
    , allocator{}
{
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(SizeType count)
    : size(count)
    , capacity(count)
    , allocator{}
{
    if (count == 0)
    {
        return;
    }

    data = AllocTraits::allocate(allocator, count);

    // 할당된 Raw 메모리 영역에 count 개수만큼 T를 기본 생성자로 초기화
    // 이 코드 이후부터 data는 유효한 T 타입 객체들로 채워짐
    std::uninitialized_value_construct_n(data, count);
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(SizeType count, const ValueType& value)
    : size(count)
    , capacity(count)
    , allocator{}
{
    if (count == 0)
    {
        return;
    }

    data = AllocTraits::allocate(allocator, count);
    std::uninitialized_fill_n(data, count, value);
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(std::initializer_list<ValueType> init_list)
    : allocator{}
{
    const SizeType count = init_list.size();
    data = AllocTraits::allocate(allocator, count);
    size = count;
    capacity = count;
    std::uninitialized_copy(init_list.begin(), init_list.end(), data);
}

template <typename T, typename Allocator>
template <std::input_iterator It>
    requires std::same_as<std::iter_value_t<It>, T>
Array<T, Allocator>::Array(It first, It last)
    : allocator{}
{
    if constexpr (std::forward_iterator<It>)
    {
        const SizeType count = std::distance(first, last);
        if (count > 0)
        {
            data = AllocTraits::allocate(allocator, count);
            std::uninitialized_copy(first, last, data);
        }
        size = count;
        capacity = count;
    }
    else
    {
        for (auto it = first; it != last; ++it)
        {
            Push(*it);
        }
    }
}

template <typename T, typename Allocator>
Array<T, Allocator>::~Array()
{
    Clear();
    AllocTraits::deallocate(allocator, data, capacity);
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(const Array& other)
    : size(other.size)
    , capacity(other.size)
    , allocator{ other.allocator }
{
    data = AllocTraits::allocate(allocator, other.size);
    std::uninitialized_copy(other.begin(), other.end(), data);
}

template <typename T, typename Allocator>
Array<T, Allocator>& Array<T, Allocator>::operator=(const Array& other)
{
    if (this != &other)
    {
        Array temp{ other };
        std::swap(*this, temp);
    }
    return *this;
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(Array&& other) noexcept
    : data(std::exchange(other.data, nullptr))
    , size(std::exchange(other.size, 0))
    , capacity(std::exchange(other.capacity, 0))
    , allocator{ std::move(other.allocator) }
{
}

template <typename T, typename Allocator>
Array<T, Allocator>& Array<T, Allocator>::operator=(Array&& other) noexcept
{
    if (this != &other)
    {
        Clear();
        AllocTraits::deallocate(allocator, data, capacity);

        data = std::exchange(other.data, nullptr);
        size = std::exchange(other.size, 0);
        capacity = std::exchange(other.capacity, 0);
        allocator = std::move(other.allocator);
    }
    return *this;
}

template <typename T, typename Allocator>
Array<T, Allocator> Array<T, Allocator>::Uninitialized(SizeType size)
    requires std::is_trivially_default_constructible_v<T>
{
    Array result;
    if (size == 0)
    {
        return result;
    }

    result.data = AllocTraits::allocate(result.allocator, size);
    result.size = size;
    result.capacity = size;

    return result;
}

template <typename T, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
Array<T, Allocator> Array<T, Allocator>::FromRange(Rng&& range)
{
    return Array{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename T, typename Allocator>
Array<T, Allocator>::SizeType Array<T, Allocator>::Len() const noexcept
{
    return size;
}

template <typename T, typename Allocator>
Array<T, Allocator>::SizeType Array<T, Allocator>::Capacity() const noexcept
{
    return capacity;
}

template <typename T, typename Allocator>
bool Array<T, Allocator>::IsEmpty() const noexcept
{
    return size == 0;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Reserve(SizeType new_capacity)
{
    EnsureCapacity(new_capacity);
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Resize(SizeType new_size)
{
    if (new_size > size)
    {
        EnsureCapacity(new_size);
        std::uninitialized_value_construct_n(data + size, new_size - size);
    }
    else if (new_size < size)
    {
        std::destroy(data + new_size, data + size);
    }
    size = new_size;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Resize(SizeType new_size, const ValueType& value)
{
    if (new_size > size)
    {
        EnsureCapacity(new_size);

        // 새로 추가된 (new_size - size)개의 원소를 value의 복사본으로 채움
        std::uninitialized_fill_n(data + size, new_size - size, value);
    }
    else if (new_size < size)
    {
        std::destroy(data + new_size, data + size);
    }
    size = new_size;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::ResizeUninitialized(SizeType new_size)
    requires std::is_trivially_default_constructible_v<T> && std::is_trivially_destructible_v<T>
{
    if (new_size > size)
    {
        EnsureCapacity(new_size);
    }
    size = new_size;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Truncate(SizeType new_size)
{
    if (new_size < size)
    {
        // 줄어든 만큼의 원소들을 제거
        std::destroy(data + new_size, data + size);
        size = new_size;
    }
}

template <typename T, typename Allocator>
void Array<T, Allocator>::ShrinkToFit()
{
    if (size < capacity)
    {
        Reallocate(size);
    }
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Clear() noexcept
{
    std::destroy(data, data + size);
    size = 0;
}

template <typename T, typename Allocator>
Optional<T&> Array<T, Allocator>::At(SizeType index)
{
    if (index >= size)
    {
        return NullOpt;
    }
    return data[index];
}

template <typename T, typename Allocator>
Optional<const T&> Array<T, Allocator>::At(SizeType index) const
{
    if (index >= size)
    {
        return NullOpt;
    }
    return data[index];
}

template <typename T, typename Allocator>
Optional<T&> Array<T, Allocator>::Front()
{
    return At(0);
}

template <typename T, typename Allocator>
Optional<const T&> Array<T, Allocator>::Front() const
{
    return At(0);
}

template <typename T, typename Allocator>
Optional<T&> Array<T, Allocator>::Back()
{
    return At(size - 1);
}

template <typename T, typename Allocator>
Optional<const T&> Array<T, Allocator>::Back() const
{
    return At(size - 1);
}

template <typename T, typename Allocator>
T* Array<T, Allocator>::Data() noexcept
{
    return data;
}

template <typename T, typename Allocator>
const T* Array<T, Allocator>::Data() const noexcept
{
    return data;
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::ValueType> Array<T, Allocator>::Pop()
{
    if (IsEmpty())
    {
        return NullOpt;
    }

    T value = std::move(BackUnsafe());
    size -= 1;
    std::destroy_at(data + size);
    return value;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Push(const ValueType& value)
{
    EnsureCapacity(size + 1);

    AllocTraits::construct(allocator, data + size, value);
    size += 1;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Push(ValueType&& value)
{
    EnsureCapacity(size + 1);

    AllocTraits::construct(allocator, data + size, std::move(value));
    size += 1;
}

template <typename T, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
void Array<T, Allocator>::Push(It first, Sent last)
{
    if (first == last)
    {
        return;
    }

    if constexpr (std::forward_iterator<It>)
    {
        const auto distance = std::distance(first, last);
        EnsureCapacity(size + distance);

        std::uninitialized_copy(first, last, data + size);
        size += distance;
    }
    else
    {
        for (auto it = first; it != last; ++it)
        {
            Push(*it);
        }
    }
}

template <typename T, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
void Array<T, Allocator>::PushRange(Rng&& range)
{
    Push(std::ranges::begin(range), std::ranges::end(range));
}

template <typename T, typename Allocator>
template <typename... Args>
Array<T, Allocator>::ValueType& Array<T, Allocator>::Emplace(Args&&... args)
{
    EnsureCapacity(size + 1);

    T* item_ptr = data + size;
    AllocTraits::construct(allocator, item_ptr, std::forward<Args>(args)...);
    size += 1;

    return *item_ptr;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Insert(SizeType index, const T& value)
{
    Insert(index, T{ value });
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Insert(SizeType index, T&& value)
{
    SE_ASSERT(index <= size, "Insert index out of bounds");

    if (index == size)
    {
        Push(std::move(value));
        return;
    }

    EnsureCapacity(size + 1);

    if constexpr (std::is_trivially_copyable_v<T>)
    {
        // 복사 비용이 없는 타입은 메모리 직접 복사(Bitwise move)로 고속 이동
        std::memmove(
            data + index + 1,           // 대상 시작: (index + 1)
            data + index,               // 소스 시작: (index)
            (size - index) * sizeof(T)  // 이동 개수: (index부터 끝까지)
        );
    }
    else
    {
        // 마지막 요소를 비어있는 다음 슬롯으로 이동 생성
        AllocTraits::construct(allocator, data + size, std::move(BackUnsafe()));

        // 나머지 요소를 뒤로 한 칸씩 이동
        std::move_backward(
            data + index,    // 소스 시작: (index)
            data + size - 1, // 소스 끝: (size - 1)
            data + size      // 대상 끝: (size)
        );
    }

    // index에 새 요소 삽입
    data[index] = std::move(value);
    size += 1;
}

template <typename T, typename Allocator>
template <std::input_iterator It>
    requires std::same_as<std::iter_value_t<It>, T>
void Array<T, Allocator>::Insert(SizeType index, It first, It last)
{
    SE_ASSERT(index <= size, "Insert index out of bounds");
    if (first == last)
    {
        return;
    }

    if constexpr (std::forward_iterator<It>)
    {
        const SizeType count = std::distance(first, last);
        EnsureCapacity(size + count);

        // 삽입 지점 뒤의 기존 원소들을 뒤로 이동시킬 공간을 생성
        if (index < size)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                // 복사 비용이 없는 타입은 메모리 직접 복사(Bitwise move)로 고속 이동
                std::memmove(
                    data + index + count,      // 대상 시작: (index + count)
                    data + index,              // 소스 시작: (index)
                    (size - index) * sizeof(T) // 이동 개수: (index부터 끝까지)
                );
            }
            else
            {
                // 이동 생성/대입이 필요한 타입은 초기화되지 않은 영역으로 소유권 이전
                std::uninitialized_move(
                    data + index,        // 소스 시작: (index)
                    data + size,         // 소스 끝: (size)
                    data + index + count // 대상 시작: (index + count)
                );
            }
        }

        // 새로운 원소들을 빈 공간에 복사
        std::uninitialized_copy(first, last, data + index);

        size += count;
    }
    else
    {
        if (Array temp{ first, last }; !temp.IsEmpty())
        {
            Insert(index, temp.begin(), temp.end());
        }
    }
}

template <typename T, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
void Array<T, Allocator>::InsertRange(SizeType index, Rng&& range)
{
    Insert(index, std::ranges::begin(range), std::ranges::end(range));
}

template <typename T, typename Allocator>
Array<T, Allocator>::SizeType Array<T, Allocator>::Remove(const ValueType& value)
{
    return RemoveIf([&value](const ValueType& element)
    {
        return element == value;
    });
}

template <typename T, typename Allocator>
void Array<T, Allocator>::RemoveAt(SizeType index)
{
    SE_ASSERT(index < size, "RemoveAt index out of bounds");

    // 제거할 요소 뒤의 모든 요소를 앞으로 한 칸씩 이동
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        // 복사 비용이 없는 타입은 메모리 직접 복사(Bitwise move)로 고속 덮어쓰기
        std::memmove(
            data + index,                  // 대상 시작: (index)
            data + index + 1,              // 소스 시작: (index + 1)
            (size - index - 1) * sizeof(T) // 이동 개수: (삭제 지점 다음부터 끝까지)
        );
    }
    else
    {
        // 이동 생성/대입이 필요한 타입은 소유권을 앞으로 이동시킨 후 마지막 슬롯 정리
        std::move(
            data + index + 1, // 소스 시작: (index + 1)
            data + size,      // 소스 끝: (size)
            data + index      // 대상 시작: (index)
        );

        // 이동 후 남겨진 마지막 요소의 소멸자 호출
        AllocTraits::destroy(allocator, data + size - 1);
    }

    size -= 1;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::RemoveRange(SizeType index, SizeType count)
{
    SE_ASSERT(index + count <= size, "RemoveRange out of bounds");
    if (count == 0)
    {
        return;
    }

    // 제거할 범위 뒤의 모든 요소를 앞으로 이동
    if constexpr (std::is_trivially_copyable_v<T>)
    {
        // 복사 비용이 없는 타입은 메모리 직접 복사(Bitwise move)로 고속 덮어쓰기
        std::memmove(
            data + index,                      // 대상 시작: (index)
            data + index + count,              // 소스 시작: (index + count)
            (size - index - count) * sizeof(T) // 이동 개수: (삭제 범위 다음부터 끝까지)
        );
    }
    else
    {
        // 이동 생성/대입이 필요한 타입은 소유권을 앞으로 이동시킨 후 남겨진 슬롯들 정리
        T* const result = std::move(
            data + index + count, // 소스 시작: (index + count)
            data + size,          // 소스 끝: (size)
            data + index          // 대상 시작: (index)
        );

        std::destroy(result, data + size);
    }
    size -= count;
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
Array<T, Allocator>::SizeType Array<T, Allocator>::RemoveIf(Predicate&& pred)
{
    // std::remove_if를 사용하여 유지할 요소들을 앞으로 이동
    const auto new_end_it = std::remove_if(begin(), end(), std::forward<Predicate>(pred));

    // 제거해야 할 요소들의 개수를 계산
    const auto num_removed = std::distance(new_end_it, end());

    if (num_removed > 0)
    {
        // 실제로 제거될 요소들의 소멸자를 호출
        std::destroy(new_end_it, end());
        size -= num_removed;
    }

    return num_removed;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::RemoveAtSwap(SizeType index)
{
    SE_ASSERT(index < size, "RemoveAtSwap index out of bounds");

    // 삭제할 위치가 마지막이 아닐 때만 덮어쓰기 수행
    if (index < size - 1)
    {
        data[index] = std::move(data[size - 1]);
    }

    size -= 1;
    AllocTraits::destroy(allocator, data + size);
}

template <typename T, typename Allocator>
bool Array<T, Allocator>::Contains(const ValueType& value) const
{
    return std::find(begin(), end(), value) != end();
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::SizeType> Array<T, Allocator>::Find(const ValueType& value) const
{
    if (const auto it = std::find(begin(), end(), value); it != end())
    {
        return std::distance(begin(), it);
    }
    return NullOpt;
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
Optional<T&> Array<T, Allocator>::FindBy(Predicate&& pred)
{
    if (auto it = std::find_if(begin(), end(), std::forward<Predicate>(pred)); it != end())
    {
        return *it;
    }
    return NullOpt;
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
Optional<const T&> Array<T, Allocator>::FindBy(Predicate&& pred) const
{
    if (auto it = std::find_if(begin(), end(), std::forward<Predicate>(pred)); it != end())
    {
        return *it;
    }
    return NullOpt;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Sort()
{
    std::stable_sort(begin(), end());
}

template <typename T, typename Allocator>
template <typename Compare>
    requires std::predicate<Compare, const T&, const T&>
void Array<T, Allocator>::Sort(Compare&& comp)
{
    std::stable_sort(begin(), end(), std::forward<Compare>(comp));
}

template <typename T, typename Allocator>
template <typename Projection>
    requires std::invocable<Projection, const T&>
void Array<T, Allocator>::SortBy(Projection&& proj)
{
    std::ranges::stable_sort(*this, {}, std::forward<Projection>(proj));
}

template <typename T, typename Allocator>
void Array<T, Allocator>::UnstableSort()
{
    std::sort(begin(), end());
}

template <typename T, typename Allocator>
template <typename Compare>
    requires std::predicate<Compare, const T&, const T&>
void Array<T, Allocator>::UnstableSort(Compare&& comp)
{
    std::sort(begin(), end(), std::forward<Compare>(comp));
}

template <typename T, typename Allocator>
template <typename Projection> requires std::invocable<Projection, const T&>
void Array<T, Allocator>::UnstableSortBy(Projection&& proj)
{
    std::ranges::sort(*this, {}, std::forward<Projection>(proj));
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Reverse()
{
    std::reverse(begin(), end());
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Swap(Array& other) noexcept
{
    std::swap(data, other.data);
    std::swap(size, other.size);
    std::swap(capacity, other.capacity);
}

template <typename T, typename Allocator>
T& Array<T, Allocator>::operator[](SizeType index) noexcept
{
    SE_ASSERT(index < size, "Index out of bounds");
    return data[index];
}

template <typename T, typename Allocator>
const T& Array<T, Allocator>::operator[](SizeType index) const noexcept
{
    SE_ASSERT(index < size, "Index out of bounds");
    return data[index];
}

template <typename T, typename Allocator>
bool Array<T, Allocator>::operator==(const Array& other) const
{
    if (size != other.size)
    {
        return false;
    }
    return std::equal(begin(), end(), other.begin());
}

template <typename T, typename Allocator>
auto Array<T, Allocator>::operator<=>(const Array& other) const
{
    return std::lexicographical_compare_three_way(
        begin(), end(),
        other.begin(), other.end()
    );
}

template <typename T, typename Allocator>
Array<T, Allocator>::IteratorType Array<T, Allocator>::begin() noexcept { return data; }

template <typename T, typename Allocator>
Array<T, Allocator>::IteratorType Array<T, Allocator>::end() noexcept { return data + size; }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstIteratorType Array<T, Allocator>::begin() const noexcept { return data; }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstIteratorType Array<T, Allocator>::end() const noexcept { return data + size; }

template <typename T, typename Allocator>
Array<T, Allocator>::ReverseIteratorType Array<T, Allocator>::rbegin() noexcept { return ReverseIteratorType{ begin() }; }

template <typename T, typename Allocator>
Array<T, Allocator>::ReverseIteratorType Array<T, Allocator>::rend() noexcept { return ReverseIteratorType{ end() }; }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstReverseIteratorType Array<T, Allocator>::rbegin() const noexcept { return ConstReverseIteratorType{ begin() }; }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstReverseIteratorType Array<T, Allocator>::rend() const noexcept { return ConstReverseIteratorType{ end() }; }

template <typename T, typename Allocator>
void Array<T, Allocator>::Reallocate(SizeType new_capacity)
{
    SE_ASSERT(new_capacity >= size, "Reallocate new_capacity must be greater than or equal to size");

    // 새로운 메모리 블록 할당
    T* new_data = AllocTraits::allocate(allocator, new_capacity);

    if (data)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            // 복사 비용이 없는 타입은 메모리 직접 복사(Bitwise copy)로 고속 이전
            std::memcpy(
                new_data,        // 대상 시작: (새 메모리)
                data,            // 소스 시작: (기존 메모리)
                size * sizeof(T) // 복사 크기: (전체 데이터)
            );
        }
        else
        {
            // 이동 생성/대입이 필요한 타입은 새로운 영역으로 소유권 이전 후 기존 객체 파괴
            std::uninitialized_move_n(data, size, new_data);
            std::destroy_n(data, size);
        }

        // 기존 메모리 블록 해제
        AllocTraits::deallocate(allocator, data, capacity);
    }

    data = new_data;
    capacity = new_capacity;
}

template <typename T, typename Allocator>
bool Array<T, Allocator>::EnsureCapacity(SizeType required_capacity)
{
    if (required_capacity > capacity)
    {
        // 재할당시 기존 capacity의 1.5배
        SizeType new_capacity = capacity + (capacity / 2);
        if (required_capacity > new_capacity)
        {
            new_capacity = required_capacity + (required_capacity / 2);
        }

        Reallocate(new_capacity);
        return true;
    }
    return false;
}

template <typename T, typename Allocator>
T& Array<T, Allocator>::FrontUnsafe()
{
    return data[0];
}

template <typename T, typename Allocator>
const T& Array<T, Allocator>::FrontUnsafe() const
{
    return data[0];
}


template <typename T, typename Allocator>
T& Array<T, Allocator>::BackUnsafe()
{
    return data[size - 1];
}

template <typename T, typename Allocator>
const T& Array<T, Allocator>::BackUnsafe() const
{
    return data[size - 1];
}
}  // namespace se
