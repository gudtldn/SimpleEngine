#pragma once

#include "SimpleEngine/Core/Container/IterChain.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"

#include <deque>
#include <initializer_list>
#include <iterator>
#include <ranges>


namespace se
{
/**
 * 양방향 큐(Double-Ended Queue) 컨테이너.
 * @detail 양쪽 끝에서의 삽입/삭제가 효율적입니다. 내부적으로 std::deque를 래핑합니다.
 * @tparam T 요소의 타입
 * @tparam Allocator 메모리 할당자 타입
 */
template <typename T, typename Allocator = DefaultAllocator<T>>
class Deque
{
private:
    using InternalDequeType = std::deque<T, Allocator>;

public:
    using ValueType = T;
    using SizeType = usize;
    using AllocatorType = Allocator;

    using IteratorType = InternalDequeType::iterator;
    using ConstIteratorType = InternalDequeType::const_iterator;
    using ReverseIteratorType = InternalDequeType::reverse_iterator;
    using ConstReverseIteratorType = InternalDequeType::const_reverse_iterator;

public:
    Deque() noexcept(noexcept(Allocator()));
    explicit Deque(SizeType count);
    Deque(SizeType count, const ValueType& value);
    Deque(std::initializer_list<ValueType> init_list);

    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, T>
    Deque(It first, Sent last);

    ~Deque() = default;
    Deque(const Deque& other) = default;
    Deque& operator=(const Deque& other) = default;
    Deque(Deque&& other) noexcept = default;
    Deque& operator=(Deque&& other) noexcept = default;

public:
    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    [[nodiscard]] static Deque FromRange(Rng&& range);

public:
    /** Deque에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** Deque이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** Deque의 크기를 변경합니다. */
    void Resize(SizeType count);
    void Resize(SizeType count, const ValueType& value);

    /** Deque의 용량을 실제 크기에 맞게 줄입니다. */
    void ShrinkToFit();

    /** 모든 요소를 제거합니다. */
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

    /** Deque의 맨 앞에 새 요소를 추가합니다. */
    T& PushFront(const ValueType& value);
    T& PushFront(ValueType&& value);

    /** Deque의 맨 뒤에 새 요소를 추가합니다. */
    T& PushBack(const ValueType& value);
    T& PushBack(ValueType&& value);

    /** Deque의 맨 앞에 새 요소를 내부 생성(emplace)합니다. */
    template <typename... Args>
    T& EmplaceFront(Args&&... args);

    /** Deque의 맨 뒤에 새 요소를 내부 생성(emplace)합니다. */
    template <typename... Args>
    T& EmplaceBack(Args&&... args);

    /** Deque의 맨 앞에서 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> PopFront();

    /** Deque의 맨 뒤에서 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> PopBack();

    /** 특정 위치에 새 요소를 삽입합니다. */
    void Insert(SizeType index, const ValueType& value);
    void Insert(SizeType index, ValueType&& value);

    template <std::input_iterator It>
        requires std::same_as<std::iter_value_t<It>, T>
    void Insert(SizeType index, It first, It last);

    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    void InsertRange(SizeType index, Rng&& range);

    /** index 위치의 요소를 제거합니다. (순서 유지) */
    void RemoveAt(SizeType index);

    /** 특정 값과 일치하는 모든 원소를 제거합니다. */
    SizeType Remove(const ValueType& value);

    /** 조건자를 만족하는 모든 원소를 제거합니다. */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    SizeType RemoveIf(Predicate&& pred);

    /** Deque에 특정 값이 포함되어 있는지 확인합니다. */
    [[nodiscard]] bool Contains(const ValueType& value) const;

    /** Deque에서 특정 값을 찾아 첫 번째로 일치하는 요소의 인덱스를 Optional로 반환합니다. */
    [[nodiscard]] Optional<SizeType> Find(const ValueType& value) const;

    /** 조건자를 만족하는 첫 번째 요소에 대한 Optional 참조를 반환합니다. */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    [[nodiscard]] Optional<T&> FindBy(Predicate&& pred);

    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    [[nodiscard]] Optional<const T&> FindBy(Predicate&& pred) const;

    /**
     * 조건자를 통과한 요소와 그렇지 않은 요소를 각각 새 Deque로 분리하여 반환합니다.
     * @tparam Predicate 요소를 인자로 받아 bool을 반환하는 함수
     * @param pred 조건자
     * @return first: 조건자를 통과한 요소들, second: 그렇지 않은 요소들
     */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    [[nodiscard]] std::pair<Deque, Deque> Partition(Predicate&& pred) const;

    /** Deque의 요소를 교환합니다. */
    void Swap(Deque& other) noexcept;

public:
    [[nodiscard]] T& operator[](SizeType index) noexcept;
    [[nodiscard]] const T& operator[](SizeType index) const noexcept;

    [[nodiscard]] bool operator==(const Deque& other) const = default;
    [[nodiscard]] auto operator<=>(const Deque& other) const = default;

    [[nodiscard]] IteratorType begin() noexcept;
    [[nodiscard]] IteratorType end() noexcept;
    [[nodiscard]] ConstIteratorType begin() const noexcept;
    [[nodiscard]] ConstIteratorType end() const noexcept;

    [[nodiscard]] ReverseIteratorType rbegin() noexcept;
    [[nodiscard]] ReverseIteratorType rend() noexcept;
    [[nodiscard]] ConstReverseIteratorType rbegin() const noexcept;
    [[nodiscard]] ConstReverseIteratorType rend() const noexcept;

    /** 요소를 순회하는 IterChain을 반환합니다. rvalue에서 호출하면 컨테이너를 소유합니다. */
    template <typename Self>
    [[nodiscard]] auto Iter(this Self&& self)
    {
        if constexpr (!std::is_reference_v<Self>)
        {
            return IterChain{ std::ranges::owning_view<std::remove_cvref_t<Self>>{ std::forward<Self>(self) } };
        }
        else
        {
            return IterChain{ std::views::all(self) };
        }
    }

    friend void swap(Deque& lhs, Deque& rhs) noexcept { lhs.Swap(rhs); }

private:
    InternalDequeType internal_deque;
};
} // namespace se

#include "SimpleEngine/Core/Container/Deque.inl"
