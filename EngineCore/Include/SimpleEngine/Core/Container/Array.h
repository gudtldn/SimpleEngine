#pragma once
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <type_traits>

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
public:
    // STL 호환성을 위해서
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = usize;
    using difference_type = isize;

    // 엔진 내부 일관성을 위한 PascalCase 별칭
    using ValueType = value_type;
    using AllocatorType = allocator_type;
    using AllocTraits = std::allocator_traits<Allocator>;
    using SizeType = size_type;
    using DifferenceType = difference_type;

    using Iterator = T*;
    using ConstIterator = const T*;
    using ReverseIterator = std::reverse_iterator<Iterator>;
    using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

public:
    Array() noexcept(noexcept(Allocator()));
    explicit Array(SizeType count);
    Array(SizeType count, const ValueType& value);
    Array(std::initializer_list<ValueType> init_list);

    template <std::input_iterator It>
    Array(It first, It last);

    template <std::ranges::input_range Rng>
    Array(Rng&& range);

    ~Array();

    Array(const Array& other);
    Array& operator=(const Array& other);
    Array(Array&& other) noexcept;
    Array& operator=(Array&& other) noexcept;

public:
    /**
     * 초기화되지 않은(uninitialized) 메모리를 가진 배열을 생성합니다.
     * @param size 배열의 크기
     * @return 생성된 Array 객체
     * @warning POD가 아닌 타입에 사용하는 것은 위험합니다. 원소를 읽기 전에 반드시 써야 합니다.
     */
    [[nodiscard]] static Array Uninitialized(SizeType size)
        requires std::is_trivially_default_constructible_v<T>;

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
    void Resize(SizeType new_size, const ValueType& value);

    /**
     * 배열의 크기를 new_size로 잘라냅니다.
     * @param new_size 새로운 배열의 크기
     * @note new_size가 현재 크기보다 크거나 같으면 아무 작업도 수행하지 않습니다.
     */
    void Truncate(SizeType new_size);

    /** 배열의 용량을 크기에 맞게 줄입니다. */
    void ShrinkToFit();

    /** 모든 요소를 제거합니다. (용량은 변경되지 않음) */
    void Clear() noexcept;

    /** 경계 검사를 수행하며 특정 인덱스의 요소에 대한 Optional 참조를 반환합니다. */
    [[nodiscard]] Optional<T&> At(SizeType index);
    [[nodiscard]] Optional<const T&> At(SizeType index) const;

    /** 첫 번째 요소에 대한 Optional 참조를 반환합니다. (비어있을 경우 nullopt) */
    [[nodiscard]] Optional<T&> Front();
    [[nodiscard]] Optional<const T&> Front() const;

    /** 마지막 요소에 대한 Optional 참조를 반환합니다. (비어있을 경우 nullopt) */
    [[nodiscard]] Optional<T&> Back();
    [[nodiscard]] Optional<const T&> Back() const;

    /** 내부 데이터 버퍼에 대한 포인터를 반환합니다. */
    [[nodiscard]] T* Data() noexcept;
    [[nodiscard]] const T* Data() const noexcept;

    /** 배열의 끝에 새 요소를 추가하고, 추가된 요소의 인덱스를 반환합니다. */
    SizeType Push(const ValueType& value);
    SizeType Push(ValueType&& value);

    /** 배열의 마지막 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> Pop();

    /** 배열의 끝에 다른 시퀀스의 모든 요소를 추가합니다. */
    template <std::input_iterator It>
    void Append(It first, It last);

    template <std::ranges::input_range Rng>
    void Append(Rng&& range);

    /** 배열의 끝에 새 요소를 내부 생성(emplace)하고, 생성된 요소의 인덱스를 반환합니다. */
    template <typename... Args>
    SizeType Emplace(Args&&... args);

    /**
     * index 위치에 새 요소를 삽입합니다.
     * @param index 삽입할 위치의 인덱스
     * @param value 삽입할 요소
     */
    void Insert(SizeType index, const T& value);
    void Insert(SizeType index, T&& value);

    /**
     * index 위치에 다른 시퀀스의 모든 요소를 삽입합니다.
     * @param index 삽입할 위치의 인덱스.
     * @param first 삽입할 시퀀스의 시작 이터레이터.
     * @param last 삽입할 시퀀스의 끝 이터레이터.
     */
    template <std::input_iterator It>
    void Insert(SizeType index, It first, It last);

    /**
     * index 위치에 다른 레인지의 모든 요소를 삽입합니다.
     * @param index 삽입할 위치의 인덱스.
     * @param range 삽입할 레인지.
     */
    template <std::ranges::input_range Rng>
    void Insert(SizeType index, Rng&& range);

    /**
     * 특정 값과 일치하는 모든 원소를 제거합니다.
     * @param value 제거할 값
     * @return 제거된 원소의 개수
     */
    SizeType Remove(const ValueType& value);

    /**
     * index 위치의 요소를 제거합니다. (순서 유지)
     * @param index 제거할 요소의 인덱스.
     */
    void RemoveAt(SizeType index);

    /**
     * index부터 count개 만큼의 요소들을 제거합니다. (순서 유지)
     * @param index 제거를 시작할 요소의 인덱스.
     * @param count 제거할 요소의 개수.
     */
    void RemoveRange(SizeType index, SizeType count);

    /**
     * 조건에 따라 배열에서 요소를 제거합니다.
     * 조건자를 만족하는 요소를 삭제하며, 삭제된 요소의 개수를 반환합니다.
     *
     * @tparam Predicate 요소에 대해 bool 값을 반환하는 함수
     * @param pred 요소를 인자로 받아 조건을 판단하는 함수
     * @return 제거된 요소의 개수
     */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    SizeType RemoveIf(Predicate&& pred);

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
    [[nodiscard]] T& operator[](SizeType index) noexcept;
    [[nodiscard]] const T& operator[](SizeType index) const noexcept;

    [[nodiscard]] bool operator==(const Array& other) const;
    [[nodiscard]] auto operator<=>(const Array& other) const;

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
    /** 메모리를 재할당 합니다. */
    void Reallocate(SizeType new_capacity);

    /**
     * 최소 required_capacity 만큼의 용량을 확보합니다. 필요하다면 재할당을 수행합니다.
     * @param required_capacity 필요한 총 용량
     */
    bool EnsureCapacity(SizeType required_capacity);

    T& FrontUnsafe();
    const T& FrontUnsafe() const;

    T& BackUnsafe();
    const T& BackUnsafe() const;

private:
    T* data = nullptr;
    SizeType size = 0;
    SizeType capacity = 0;
    [[no_unique_address]] AllocatorType allocator;
};
}

#include "SimpleEngine/Core/Container/Array.inl"
