#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"

#include <functional>
#include <unordered_set>


namespace se
{
/**
 * 해시 테이블 기반의 고유 원소 집합 컨테이너
 * @tparam T 요소의 타입
 * @tparam Hasher 해시 함수의 타입
 * @tparam KeyEq 키 비교 함수의 타입
 * @tparam Allocator 메모리 할당자 타입
 * @todo 나중에 필요하면 SetEntry 구현
 */
template <
    typename T,
    typename Hasher = std::hash<T>,
    typename KeyEq = std::equal_to<T>,
    typename Allocator = DefaultAllocator<T>
>
class HashSet
{
private:
    using InternalSetType = std::unordered_set<T, Hasher, KeyEq, Allocator>;

public:
    using ValueType = T;
    using SizeType = usize;

    using IteratorType = InternalSetType::iterator;
    using ConstIteratorType = InternalSetType::const_iterator;

public:
    HashSet() = default;
    explicit HashSet(SizeType capacity);
    HashSet(std::initializer_list<ValueType> init_list);

    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, T>
    HashSet(It first, Sent last);

    ~HashSet() = default;
    HashSet(const HashSet&) = default;
    HashSet& operator=(const HashSet&) = default;
    HashSet(HashSet&&) noexcept = default;
    HashSet& operator=(HashSet&&) noexcept = default;

public:
    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    [[nodiscard]] static HashSet FromRange(Rng&& range);

public:
    /** Set에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** Set이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** 재할당 없이 Set이 담을 수 있는 요소의 수를 반환합니다. (내부 버킷 수) */
    [[nodiscard]] SizeType Capacity() const noexcept;

    /** 최소 new_capacity 만큼의 요소를 저장할 수 있도록 용량을 예약합니다. */
    void Reserve(SizeType new_capacity);

    /** Set의 모든 요소를 제거하여 비웁니다. */
    void Clear() noexcept;

    /**
     * 새로운 요소를 Set에 추가합니다.
     * 요소가 이미 Set에 존재하는 경우에는 아무 작업도 수행하지 않습니다.
     * @param value 추가할 요소
     * @return 새로운 요소가 실제로 추가되었으면 true, 이미 존재했다면 false
     */
    bool Insert(const ValueType& value);
    bool Insert(ValueType&& value);

    /**
     * 새로운 요소를 Set에 내부 생성(emplace)하여 추가합니다.
     * @tparam Args 요소의 생성자에 전달할 인수들의 타입
     * @param args 요소의 생성자에 전달할 인수들
     * @return 새로운 요소가 실제로 추가되었으면 true, 이미 존재했다면 false
     */
    template <typename... Args>
    bool Emplace(Args&&... args);

    /**
     * 지정된 값을 가진 요소를 Set에서 제거합니다.
     * @param value 제거할 요소의 값
     * @return 요소가 성공적으로 제거되었으면 true, 해당 요소가 Set에 없었으면 false
     */
    bool Remove(const ValueType& value);

    /**
     * 주어진 조건자를 만족하는 모든 요소를 Set에서 제거합니다.
     * @tparam Predicate 조건자의 타입
     * @param pred bool을 반환하고 const ValueType&를 인자로 받는 조건자
     * @return 제거된 요소의 개수
     */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    SizeType RemoveIf(Predicate&& pred);

    /**
     * 지정된 값이 Set에 포함되어 있는지 확인합니다.
     * @param value 확인할 값
     * @return 요소가 Set에 존재하면 true, 그렇지 않으면 false
     */
    [[nodiscard]] bool Contains(const ValueType& value) const;

    /**
     * Set에 포함된 모든 요소를 담은 새로운 Array를 생성하여 반환합니다.
     * @return 모든 요소가 담긴 se::Array<ValueType> 객체
     */
    [[nodiscard]] Array<ValueType> ToArray() const;

    void Swap(HashSet& other) noexcept;

public:
    [[nodiscard]] bool operator==(const HashSet& other) const = default;

    // Iterator
    [[nodiscard]] IteratorType begin() noexcept;
    [[nodiscard]] IteratorType end() noexcept;
    [[nodiscard]] ConstIteratorType begin() const noexcept;
    [[nodiscard]] ConstIteratorType end() const noexcept;

    friend void swap(HashSet& lhs, HashSet& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    InternalSetType internal_set;
};
} // namespace se

#include "SimpleEngine/Core/Container/HashSet.inl"
