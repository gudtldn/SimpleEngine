#pragma once
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <vector>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"


namespace se
{
/**
 * 동적 크기를 가지는 연속 메모리 기반 배열 컨테이너
 * @tparam T 요소의 타입
 * @tparam Allocator 메모리 할당자 타입
 */
template <typename T, typename Allocator = core::memory::DefaultAllocator<T>>
class Array
{
private:
    using InternalVectorType = std::vector<T, Allocator>;

public:
    // STL 호환성을 위해서
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = usize;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = std::allocator_traits<Allocator>::pointer;
    using const_pointer = std::allocator_traits<Allocator>::const_pointer;

    // 엔진 내부 일관성을 위한 PascalCase 별칭
    using ValueType = value_type;
    using AllocatorType = allocator_type;
    using SizeType = size_type;
    using DifferenceType = difference_type;
    using Reference = reference;
    using ConstReference = const_reference;
    using Pointer = pointer;
    using ConstPointer = const_pointer;

    using Iterator = InternalVectorType::iterator;
    using ConstIterator = InternalVectorType::const_iterator;
    using ReverseIterator = InternalVectorType::reverse_iterator;
    using ConstReverseIterator = InternalVectorType::const_reverse_iterator;

public:
    Array() noexcept(noexcept(Allocator()));
    explicit Array(const AllocatorType& allocator) noexcept;
    explicit Array(SizeType count, const AllocatorType& allocator = AllocatorType{});
    Array(SizeType count, const ValueType& value, const AllocatorType& allocator = AllocatorType{});
    Array(std::initializer_list<ValueType> init_list, const AllocatorType& allocator = AllocatorType{});

    template <std::input_iterator It>
    Array(It first, It last, const AllocatorType& allocator = AllocatorType{});

    ~Array() = default;
    Array(const Array& other) = default;
    Array& operator=(const Array& other) = default;
    Array(Array&& other) noexcept = default;
    Array& operator=(Array&& other) noexcept = default;

public:
    /** 배열에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** 재할당 없이 배열이 담을 수 있는 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Capacity() const noexcept;

    /** 배열이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** 최소 new_capacity 만큼의 요소를 저장할 수 있도록 용량을 예약합니다. */
    void Reserve(SizeType new_capacity);

    /** 배열의 크기를 new_size로 변경합니다. */
    void Resize(SizeType new_size);

    /** 배열의 용량을 크기에 맞게 줄입니다. */
    void ShrinkToFit();

    /** 모든 요소를 제거합니다. (용량은 변경되지 않음) */
    void Clear() noexcept;

    /** 경계 검사를 수행하며 특정 인덱스의 요소에 대한 Optional 참조를 반환합니다. */
    [[nodiscard]] Optional<Reference> At(SizeType index);
    [[nodiscard]] Optional<ConstReference> At(SizeType index) const;

    /** 첫 번째 요소에 대한 Optional 참조를 반환합니다. (비어있을 경우 nullopt) */
    [[nodiscard]] Optional<Reference> Front();
    [[nodiscard]] Optional<ConstReference> Front() const;

    /** 마지막 요소에 대한 Optional 참조를 반환합니다. (비어있을 경우 nullopt) */
    [[nodiscard]] Optional<Reference> Back();
    [[nodiscard]] Optional<ConstReference> Back() const;

    /** 내부 데이터 버퍼에 대한 포인터를 반환합니다. */
    [[nodiscard]] Pointer Data() noexcept;
    [[nodiscard]] ConstPointer Data() const noexcept;

    /** 배열의 끝에 새 요소를 추가하고, 추가된 요소의 참조를 반환합니다. */
    Reference Add(const ValueType& value);
    Reference Add(ValueType&& value);

    /** 배열의 끝에 새 요소를 내부 생성(emplace)하고, 생성된 요소의 참조를 반환합니다. */
    template <typename... Args>
    Reference Emplace(Args&&... args);

    /** 배열의 마지막 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> Pop();

    /**
     * 특정 인덱스의 요소를 제거합니다. (순서 보장 안됨)
     * 제거할 요소를 마지막 요소와 교체한 뒤 마지막 요소를 제거합니다.
     * @return 작업 성공 여부
     */
    bool RemoveAtSwap(SizeType index);

    /** 배열에 특정 값이 포함되어 있는지 확인합니다. */
    [[nodiscard]] bool Contains(const ValueType& value) const;

    /** 배열에서 특정 값을 찾아 첫 번째로 일치하는 요소의 인덱스를 Optional로 반환합니다. */
    [[nodiscard]] Optional<SizeType> Find(const ValueType& value) const;

public:
    /** 경계 검사 없이 특정 인덱스의 요소에 접근합니다. */
    [[nodiscard]] Reference operator[](SizeType index) noexcept;
    [[nodiscard]] ConstReference operator[](SizeType index) const noexcept;

    // Iterator
    [[nodiscard]] Iterator begin() noexcept;
    [[nodiscard]] Iterator end() noexcept;
    [[nodiscard]] ConstIterator begin() const noexcept;
    [[nodiscard]] ConstIterator end() const noexcept;

    [[nodiscard]] ReverseIterator rbegin() noexcept;
    [[nodiscard]] ReverseIterator rend() noexcept;
    [[nodiscard]] ConstReverseIterator rbegin() const noexcept;
    [[nodiscard]] ConstReverseIterator rend() const noexcept;

private:
    InternalVectorType internal_vector;
};


template <typename T, typename Allocator>
Array<T, Allocator>::Array() noexcept(noexcept(Allocator()))
    : internal_vector()
{
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(const AllocatorType& allocator) noexcept
    : internal_vector(allocator)
{
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(SizeType count, const AllocatorType& allocator)
    : internal_vector(count, allocator)
{
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(SizeType count, const ValueType& value, const AllocatorType& allocator)
    : internal_vector(count, value, allocator)
{
}

template <typename T, typename Allocator>
Array<T, Allocator>::Array(std::initializer_list<ValueType> init_list, const AllocatorType& allocator)
    : internal_vector(init_list, allocator)
{
}

template <typename T, typename Allocator>
template <std::input_iterator It>
Array<T, Allocator>::Array(It first, It last, const AllocatorType& allocator)
    : internal_vector(first, last, allocator)
{
}

template <typename T, typename Allocator>
Array<T, Allocator>::SizeType Array<T, Allocator>::Len() const noexcept
{
    return internal_vector.size();
}

template <typename T, typename Allocator>
Array<T, Allocator>::SizeType Array<T, Allocator>::Capacity() const noexcept
{
    return internal_vector.capacity();
}

template <typename T, typename Allocator>
bool Array<T, Allocator>::IsEmpty() const noexcept
{
    return internal_vector.empty();
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Reserve(SizeType new_capacity)
{
    internal_vector.reserve(new_capacity);
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Resize(SizeType new_size)
{
    internal_vector.resize(new_size);
}

template <typename T, typename Allocator>
void Array<T, Allocator>::ShrinkToFit()
{
    internal_vector.shrink_to_fit();
}

template <typename T, typename Allocator>
void Array<T, Allocator>::Clear() noexcept
{
    internal_vector.clear();
}

template <typename T, typename Allocator>
Array<T, Allocator>::Reference Array<T, Allocator>::operator[](SizeType index) noexcept
{
    assert(index < Len() && "Index out of bounds");
    return internal_vector[index];
}

template <typename T, typename Allocator>
Array<T, Allocator>::ConstReference Array<T, Allocator>::operator[](SizeType index) const noexcept
{
    assert(index < Len() && "Index out of bounds");
    return internal_vector[index];
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::Reference> Array<T, Allocator>::At(SizeType index)
{
    if (index >= Len())
    {
        return std::nullopt;
    }
    return internal_vector[index];
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::ConstReference> Array<T, Allocator>::At(SizeType index) const
{
    if (index >= Len())
    {
        return std::nullopt;
    }
    return internal_vector[index];
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::Reference> Array<T, Allocator>::Front()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return internal_vector.front();
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::ConstReference> Array<T, Allocator>::Front() const
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return internal_vector.front();
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::Reference> Array<T, Allocator>::Back()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return internal_vector.back();
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::ConstReference> Array<T, Allocator>::Back() const
{
    if (IsEmpty())
    {
        return std::nullopt;
    }
    return internal_vector.back();
}

template <typename T, typename Allocator>
Array<T, Allocator>::Pointer Array<T, Allocator>::Data() noexcept
{
    return internal_vector.data();
}

template <typename T, typename Allocator>
Array<T, Allocator>::ConstPointer Array<T, Allocator>::Data() const noexcept
{
    return internal_vector.data();
}

template <typename T, typename Allocator>
Array<T, Allocator>::Reference Array<T, Allocator>::Add(const ValueType& value)
{
    internal_vector.push_back(value);
    return internal_vector.back();
}

template <typename T, typename Allocator>
Array<T, Allocator>::Reference Array<T, Allocator>::Add(ValueType&& value)
{
    internal_vector.push_back(std::move(value));
    return internal_vector.back();
}

template <typename T, typename Allocator>
template <typename... Args>
Array<T, Allocator>::Reference Array<T, Allocator>::Emplace(Args&&... args)
{
    return internal_vector.emplace_back(std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
Optional<typename Array<T, Allocator>::ValueType> Array<T, Allocator>::Pop()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    ValueType value = std::move(internal_vector.back());
    internal_vector.pop_back();
    return { std::move(value) };
}

template <typename T, typename Allocator>
bool Array<T, Allocator>::RemoveAtSwap(SizeType index)
{
    if (index >= Len())
    {
        return false;
    }

    // 마지막 요소가 아닌 경우에만 swap
    if (index != Len() - 1)
    {
        std::swap((*this)[index], Back().Value());
    }

    internal_vector.pop_back();
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
Array<T, Allocator>::Iterator Array<T, Allocator>::begin() noexcept { return internal_vector.begin(); }

template <typename T, typename Allocator>
Array<T, Allocator>::Iterator Array<T, Allocator>::end() noexcept { return internal_vector.end(); }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstIterator Array<T, Allocator>::begin() const noexcept { return internal_vector.begin(); }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstIterator Array<T, Allocator>::end() const noexcept { return internal_vector.end(); }

template <typename T, typename Allocator>
Array<T, Allocator>::ReverseIterator Array<T, Allocator>::rbegin() noexcept { return internal_vector.rbegin(); }

template <typename T, typename Allocator>
Array<T, Allocator>::ReverseIterator Array<T, Allocator>::rend() noexcept { return internal_vector.rend(); }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstReverseIterator Array<T, Allocator>::rbegin() const noexcept { return internal_vector.rbegin(); }

template <typename T, typename Allocator>
Array<T, Allocator>::ConstReverseIterator Array<T, Allocator>::rend() const noexcept { return internal_vector.rend(); }
}
