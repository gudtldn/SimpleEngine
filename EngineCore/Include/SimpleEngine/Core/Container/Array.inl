#pragma once
#include <algorithm>
#include <memory>
#include <utility>


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
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
Array<T, Allocator> Array<T, Allocator>::FromRange(Rng&& range)
{
    return Array{ std::ranges::begin(range), std::ranges::end(range) };
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
        return std::nullopt;
    }
    return data[index];
}

template <typename T, typename Allocator>
Optional<const T&> Array<T, Allocator>::At(SizeType index) const
{
    if (index >= size)
    {
        return std::nullopt;
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
Array<T, Allocator>::SizeType Array<T, Allocator>::Push(const ValueType& value)
{
    EnsureCapacity(size + 1);

    AllocTraits::construct(allocator, data + size, value);
    return size++;
}

template <typename T, typename Allocator>
Array<T, Allocator>::SizeType Array<T, Allocator>::Push(ValueType&& value)
{
    EnsureCapacity(size + 1);

    AllocTraits::construct(allocator, data + size, std::move(value));
    return size++;
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::ValueType> Array<T, Allocator>::Pop()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    T value = std::move(BackUnsafe());
    --size;
    std::destroy_at(data + size);
    return value;
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
Array<T, Allocator>::SizeType Array<T, Allocator>::Emplace(Args&&... args)
{
    EnsureCapacity(size + 1);

    AllocTraits::construct(allocator, data + size, std::forward<Args>(args)...);
    return size++;
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Insert(SizeType index, const T& value)
{
    Insert(index, T{ value });
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Insert(SizeType index, T&& value)
{
    assert(index <= size && "Insert index out of bounds");

    if (index == size)
    {
        Push(std::move(value));
        return;
    }

    EnsureCapacity(size + 1);

    // 마지막 요소를 비어있는 다음 슬롯으로 이동 생성
    AllocTraits::construct(allocator, data + size, std::move(BackUnsafe()));

    // 나머지 요소를 뒤로 한 칸씩 이동
    std::move_backward(
        data + index,    // 소스 시작: (index)
        data + size - 1, // 소스 끝: (size - 1)
        data + size      // 대상 끝: (size)
    );

    // index에 새 요소 삽입
    data[index] = std::move(value);
    ++size;
}

template <typename T, typename Allocator>
template <std::input_iterator It>
    requires std::same_as<std::iter_value_t<It>, T>
void Array<T, Allocator>::Insert(SizeType index, It first, It last)
{
    assert(index <= size && "Insert index out of bounds");
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
            std::uninitialized_move(
                data + index,
                data + size,
                data + index + count
            );
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
    assert(index < size && "RemoveAt index out of bounds");

    // 제거할 요소 뒤의 모든 요소를 앞으로 한 칸씩 이동
    std::move(data + index + 1, data + size, data + index);

    --size;
    AllocTraits::destroy(allocator, data + size);
}

template <typename T, typename Allocator>
void Array<T, Allocator>::RemoveRange(SizeType index, SizeType count)
{
    assert(index + count <= size && "RemoveRange out of bounds");
    if (count == 0)
    {
        return;
    }

    // 제거할 범위 뒤의 모든 요소를 앞으로 이동
    T* const result = std::move(
        data + index + count,
        data + size,
        data + index
    );

    size -= count;
    std::destroy(result, data + size);
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
bool Array<T, Allocator>::RemoveAtSwap(SizeType index)
{
    if (index >= size)
    {
        return false;
    }

    // 마지막 요소가 아닌 경우에만 swap
    if (index != size - 1)
    {
        std::swap(data[index], BackUnsafe());
    }

    --size;
    std::destroy_at(data + size);

    return true;
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
    return std::nullopt;
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
    assert(index < size && "Index out of bounds");
    return data[index];
}

template <typename T, typename Allocator>
const T& Array<T, Allocator>::operator[](SizeType index) const noexcept
{
    assert(index < size && "Index out of bounds");
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
    assert(new_capacity >= size && "Reallocate new_capacity must be greater than or equal to size");

    T* new_data = AllocTraits::allocate(allocator, new_capacity);

    if (data)
    {
        std::uninitialized_move_n(data, size, new_data);
        std::destroy_n(data, size);
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
        SizeType new_capacity = capacity + capacity / 2;
        if (required_capacity > new_capacity)
        {
            new_capacity = required_capacity + required_capacity / 2;
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
}
