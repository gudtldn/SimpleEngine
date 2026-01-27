#pragma once
#include <functional>
#include <set>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se
{
/**
 * 트리 기반의 고유 원소 집합 컨테이너
 * @tparam T 요소의 타입
 * @tparam Pred 키 비교 함수의 타입
 * @tparam Allocator 메모리 할당자 타입
 * @todo 나중에 필요하면 SetEntry 구현
 */
template <
    typename T,
    typename Pred = std::less<T>,
    typename Allocator = core::DefaultAllocator<T>
>
class Set
{
private:
    using InternalSetType = std::set<T, Pred, Allocator>;

public:
    using value_type = T;
    using size_type = usize;

    using ValueType = value_type;
    using SizeType = size_type;

    using Iterator = InternalSetType::iterator;
    using ConstIterator = InternalSetType::const_iterator;

public:
    Set() = default;
    Set(std::initializer_list<ValueType> init_list);

    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, T>
    Set(It first, Sent last);

    ~Set() = default;
    Set(const Set&) = default;
    Set& operator=(const Set&) = default;
    Set(Set&&) noexcept = default;
    Set& operator=(Set&&) noexcept = default;

public:
    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    [[nodiscard]] static Set FromRange(Rng&& range);

public:
    /** Set에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** Set이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

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

    void Swap(Set& other) noexcept;

public:
    [[nodiscard]] bool operator==(const Set& other) const = default;
    [[nodiscard]] auto operator<=>(const Set& other) const = default;

    // Iterator
    [[nodiscard]] Iterator begin() noexcept;
    [[nodiscard]] Iterator end() noexcept;
    [[nodiscard]] ConstIterator begin() const noexcept;
    [[nodiscard]] ConstIterator end() const noexcept;

    friend void swap(Set& lhs, Set& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    InternalSetType internal_set;
};

template <typename T>
core::Archive& operator<<(core::Archive& ar, Set<T>& set)
{
    uint64 size = set.Len();
    ar.BeginArray(size);

    if (ar.IsLoading())
    {
        set.Clear();
        for (uint64 i = 0; i < size; ++i)
        {
            T value;
            ar << value;

            set.Emplace(std::move(value));
        }
    }
    else
    {
        for (const T& value : set)
        {
            ar << const_cast<T&>(value);
        }
    }

    ar.EndArray();
    return ar;
}
}  // namespace se

#include "SimpleEngine/Core/Container/Set.inl"
