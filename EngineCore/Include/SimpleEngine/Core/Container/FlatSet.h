#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * 정렬된 Array 기반의 고유 원소 집합 컨테이너
 * @tparam T 요소의 타입
 * @tparam Pred 키 비교 함수의 타입
 * @tparam Container 내부 저장 컨테이너 타입
 */
template <
    typename T,
    typename Pred = std::less<T>,
    typename Container = Array<T>
>
class FlatSet
{
public:
    using ValueType = T;
    using SizeType = usize;

    using IteratorType = Container::IteratorType;
    using ConstIteratorType = Container::ConstIteratorType;
    using ReverseIteratorType = Container::ReverseIteratorType;
    using ConstReverseIteratorType = Container::ConstReverseIteratorType;

public:
    FlatSet() = default;
    FlatSet(std::initializer_list<ValueType> init_list);

    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, T>
    FlatSet(It first, Sent last);

    ~FlatSet() = default;
    FlatSet(const FlatSet&) = default;
    FlatSet& operator=(const FlatSet&) = default;
    FlatSet(FlatSet&&) noexcept = default;
    FlatSet& operator=(FlatSet&&) noexcept = default;

public:
    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    [[nodiscard]] static FlatSet FromRange(Rng&& range);

public:
    /** FlatSet에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** FlatSet이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** 재할당 없이 FlatSet이 담을 수 있는 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Capacity() const noexcept;

    /** FlatSet의 모든 요소를 제거하여 비웁니다. */
    void Clear() noexcept;

    /**
     * 새로운 요소를 FlatSet에 추가합니다.
     * 요소가 이미 FlatSet에 존재하는 경우에는 아무 작업도 수행하지 않습니다.
     * @param value 추가할 요소
     * @return 새로운 요소가 실제로 추가되었으면 true, 이미 존재했다면 false
     */
    bool Insert(const ValueType& value);
    bool Insert(ValueType&& value);

    /**
     * 새로운 요소를 FlatSet에 내부 생성(emplace)하여 추가합니다.
     * @tparam Args 요소의 생성자에 전달할 인수들의 타입
     * @param args 요소의 생성자에 전달할 인수들
     * @return 새로운 요소가 실제로 추가되었으면 true, 이미 존재했다면 false
     */
    template <typename... Args>
    bool Emplace(Args&&... args);

    /**
     * 지정된 값을 가진 요소를 FlatSet에서 제거합니다.
     * @param value 제거할 요소의 값
     * @return 요소가 성공적으로 제거되었으면 true, 해당 요소가 FlatSet에 없었으면 false
     */
    bool Remove(const ValueType& value);

    /**
     * 주어진 조건자를 만족하는 모든 요소를 FlatSet에서 제거합니다.
     * @tparam Predicate 조건자의 타입
     * @param pred bool을 반환하고 const ValueType&를 인자로 받는 조건자
     * @return 제거된 요소의 개수
     */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    SizeType RemoveIf(Predicate&& pred);

    /**
     * 지정된 값이 FlatSet에 포함되어 있는지 확인합니다.
     * @param value 확인할 값
     * @return 요소가 FlatSet에 존재하면 true, 그렇지 않으면 false
     */
    [[nodiscard]] bool Contains(const ValueType& value) const;

    /**
     * 내부 배열에 대한 참조를 반환합니다.
     * @return 정렬된 내부 배열의 참조
     */
    [[nodiscard]] const Container& GetArray() const noexcept;

    /**
     * FlatSet에 포함된 모든 요소를 담은 새로운 Array를 생성하여 반환합니다.
     * @return 모든 요소가 담긴 se::Array<ValueType> 객체
     */
    [[nodiscard]] Array<ValueType> ToArray() const;

    /** 최소 new_capacity 만큼의 요소를 저장할 수 있도록 용량을 예약합니다. */
    void Reserve(SizeType new_capacity);

    /** 배열의 용량을 크기에 맞게 줄입니다. */
    void ShrinkToFit();

    void Swap(FlatSet& other) noexcept;

public:
    [[nodiscard]] bool operator==(const FlatSet& other) const = default;
    [[nodiscard]] auto operator<=>(const FlatSet& other) const = default;

    // Iterator
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

    friend void swap(FlatSet& lhs, FlatSet& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    Container internal_array;
    NO_UNIQUE_ADDRESS Pred compare;
};
} // namespace se

#include "SimpleEngine/Core/Container/FlatSet.inl"
