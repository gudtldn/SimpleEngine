#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FlatMapEntry.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"


namespace se
{
/**
 * 정렬된 Array 기반의 Key-Value 컨테이너
 *
 * @tparam Key 키의 타입
 * @tparam Value 값의 타입
 * @tparam Pred 키 비교 함수의 타입
 * @tparam Allocator 메모리 할당자 타입
 */
template <
    typename Key,
    typename Value,
    typename Pred = std::less<Key>,
    typename Allocator = DefaultAllocator<std::pair<Key, Value>>
>
class FlatMap
{
private:
    friend class FlatMapEntry<FlatMap>;

public:
    using KeyType = Key;
    using ValueType = Value;
    using PairType = std::pair<Key, Value>;
    using SizeType = usize;

    using IteratorType = Array<PairType, Allocator>::IteratorType;
    using ConstIteratorType = Array<PairType, Allocator>::ConstIteratorType;

    using EntryType = FlatMapEntry<FlatMap>;

    // STL 호환성을 위한 별칭
    using key_type = Key;
    using mapped_type = Value;
    using value_type = PairType;
    using size_type = usize;
    using iterator = IteratorType;
    using const_iterator = ConstIteratorType;

public:
    FlatMap() = default;
    FlatMap(std::initializer_list<PairType> init_list);

    template <std::input_iterator It, std::sentinel_for<It> Sent>
    FlatMap(It first, Sent last);

    ~FlatMap() = default;
    FlatMap(const FlatMap&) = default;
    FlatMap& operator=(const FlatMap&) = default;
    FlatMap(FlatMap&&) noexcept = default;
    FlatMap& operator=(FlatMap&&) noexcept = default;

public:
    template <std::ranges::input_range Rng>
    [[nodiscard]] static FlatMap FromRange(Rng&& range);

public:
    /** FlatMap에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** FlatMap이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** FlatMap의 모든 요소를 제거합니다. */
    void Clear() noexcept;

    /**
     * 새로운 요소를 FlatMap에 추가합니다.
     * 이미 동일한 키를 가진 요소가 존재할 경우, 해당 요소의 값을 대체합니다.
     * @param key 삽입 또는 업데이트할 키
     * @param value 키에 매핑할 값
     * @return 삽입되거나 업데이트된 값에 대한 참조
     */
    template <typename V = ValueType>
        requires std::constructible_from<Value, V&&>
    ValueType& Insert(const KeyType& key, V&& value);

    template <typename V = ValueType>
        requires std::constructible_from<Value, V&&>
    ValueType& Insert(KeyType&& key, V&& value);

    /**
     * 새로운 요소를 FlatMap에 내부 생성(emplace)하여 추가합니다.
     * @tparam Args 요소의 생성자에 전달할 인수들의 타입
     * @param key 삽입할 키
     * @param args 요소의 생성자에 전달할 인수들
     */
    template <typename... Args>
        requires std::constructible_from<Value, Args&&...>
    ValueType& Emplace(const KeyType& key, Args&&... args);

    template <typename... Args>
        requires std::constructible_from<Value, Args&&...>
    ValueType& Emplace(KeyType&& key, Args&&... args);

    /**
     * 특정 키에 대한 Entry 객체를 반환합니다.
     * @param key 검색 또는 삽입할 키
     * @return 키의 존재 여부에 따라 분기된 Entry 객체
     */
    [[nodiscard]] EntryType Entry(const KeyType& key);
    [[nodiscard]] EntryType Entry(KeyType&& key);

    /**
     * Key에 해당하는 값을 찾습니다.
     * @param key 검색할 Key
     * @return 값에 대한 Optional 참조 (Key가 없으면 nullopt)
     */
    [[nodiscard]] Optional<ValueType&> Find(const KeyType& key);
    [[nodiscard]] Optional<const ValueType&> Find(const KeyType& key) const;

    /**
     * Key에 해당하는 값을 찾습니다.
     * @warning 키가 존재하지 않으면 SE_UNREACHABLE()을 통해 프로그램이 종료됩니다.
     *
     * @param key 검색할 Key
     * @return 값에 대한 참조
     */
    [[nodiscard]] ValueType& FindChecked(const KeyType& key);
    [[nodiscard]] const ValueType& FindChecked(const KeyType& key) const;

    /** 가장 작은 키를 가진 요소의 참조를 Optional로 반환합니다. */
    [[nodiscard]] Optional<PairType&> First() noexcept;
    [[nodiscard]] Optional<const PairType&> First() const noexcept;

    /** 가장 큰 키를 가진 요소의 참조를 Optional로 반환합니다. */
    [[nodiscard]] Optional<PairType&> Last() noexcept;
    [[nodiscard]] Optional<const PairType&> Last() const noexcept;

    /**
     * 주어진 키보다 크거나 같은 첫 번째 요소를 찾습니다.
     * @return 키-값 쌍에 대한 Optional 참조
     */
    [[nodiscard]] Optional<PairType&> LowerBoundEntry(const KeyType& key);
    [[nodiscard]] Optional<const PairType&> LowerBoundEntry(const KeyType& key) const;

    /**
     * 주어진 키보다 큰 첫 번째 요소를 찾습니다.
     * @return 키-값 쌍에 대한 Optional 참조
     */
    [[nodiscard]] Optional<PairType&> UpperBoundEntry(const KeyType& key);
    [[nodiscard]] Optional<const PairType&> UpperBoundEntry(const KeyType& key) const;

    /**
     * 특정 Key가 FlatMap에 포함되어 있는지 확인합니다
     * @param key 확인할 Key
     */
    [[nodiscard]] bool Contains(const KeyType& key) const;

    /**
     * 특정 키를 가진 요소를 제거합니다.
     * @param key 제거할 키
     * @return 제거에 성공하면 true, 해당 키가 없으면 false
     */
    bool Remove(const KeyType& key);

    /**
     * 조건자를 만족하는 모든 요소를 제거합니다.
     * @param pred bool을 반환하고 Key-Value를 인자로 받는 조건자
     * @return 제거된 요소의 개수
     */
    template <typename Predicate>
        requires std::predicate<Predicate, const Key&, const Value&>
    SizeType RemoveIf(Predicate&& pred);

    /** FlatMap의 모든 키를 담은 Array를 생성하여 반환합니다. */
    template <typename T = KeyType>
        requires std::constructible_from<T, const Key&>
    [[nodiscard]] Array<KeyType> Keys() const;

    /** FlatMap의 모든 값을 담은 Array를 생성하여 반환합니다. */
    template <typename T = ValueType>
        requires std::constructible_from<T, const Value&>
    [[nodiscard]] Array<ValueType> Values() const;

    /** 최소 new_capacity 만큼의 요소를 저장할 수 있도록 용량을 예약합니다. */
    void Reserve(SizeType new_capacity);

    /** 배열의 용량을 크기에 맞게 줄입니다. */
    void ShrinkToFit();

    void Swap(FlatMap& other) noexcept;

public:
    /**
     * 키에 해당하는 값에 접근합니다. 키가 없으면 기본 생성된 값을 삽입 후 반환합니다.
     * @param key 접근할 키
     */
    [[nodiscard]] ValueType& operator[](const KeyType& key);

    [[nodiscard]] bool operator==(const FlatMap&) const = default;
    [[nodiscard]] auto operator<=>(const FlatMap&) const = default;

    // Iterator
    [[nodiscard]] IteratorType begin() noexcept;
    [[nodiscard]] IteratorType end() noexcept;
    [[nodiscard]] ConstIteratorType begin() const noexcept;
    [[nodiscard]] ConstIteratorType end() const noexcept;

    friend void swap(FlatMap& lhs, FlatMap& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    struct PairCompare
    {
        [[no_unique_address]] Pred compare;
        bool operator()(const PairType& lhs, const KeyType& rhs) const { return compare(lhs.first, rhs); }
        bool operator()(const KeyType& lhs, const PairType& rhs) const { return compare(lhs, rhs.first); }
        bool operator()(const PairType& lhs, const PairType& rhs) const { return compare(lhs.first, rhs.first); }
    };

    Array<PairType, Allocator> internal_array;
    [[no_unique_address]] PairCompare pair_compare;
};
}  // namespace se

#include "SimpleEngine/Core/Container/FlatMap.inl"
