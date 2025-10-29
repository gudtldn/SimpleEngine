#pragma once
#include <functional>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <map>
#include <utility>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/MapEntry.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"


namespace se
{
/**
 * Red-Black Tree 기반의 Key-Value 컨테이너
 *
 * @tparam Key 키의 타입
 * @tparam Value 값의 타입
 * @tparam Pred 해시 함수의 타입
 * @tparam Allocator 메모리 할당자 타입
 */
template <
    typename Key,
    typename Value,
    typename Pred = std::less<Key>,
    typename Allocator = core::memory::DefaultAllocator<std::pair<const Key, Value>>
>
class Map
{
private:
    friend class MapEntry<Map>;

    using InternalMapType = std::map<Key, Value, Pred, Allocator>;
    using PairType = std::pair<const Key, Value>;

public:
    // STL 호환성을 위한 별칭
    using key_type = Key;
    using mapped_type = Value;
    using value_type = PairType;
    using size_type = usize;

    // 엔진 내부 일관성을 위한 PascalCase 별칭
    using KeyType = key_type;
    using ValueType = mapped_type;
    using SizeType = size_type;

    using IteratorType = InternalMapType::iterator;
    using ConstIteratorType = InternalMapType::const_iterator;

    using EntryType = MapEntry<Map>;

public:
    Map() = default;
    Map(std::initializer_list<PairType> init_list);

    template <std::input_iterator It>
    Map(It first, It last);

    ~Map() = default;
    Map(const Map&) = default;
    Map& operator=(const Map&) = default;
    Map(Map&&) noexcept = default;
    Map& operator=(Map&&) noexcept = default;

public:
    template <std::ranges::input_range Rng>
    [[nodiscard]] static Map FromRange(Rng&& range);

public:
    /** Map에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** Map이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** Map의 모든 요소를 제거합니다. */
    void Clear() noexcept;

    /** TODO: docs */
    template <typename... Args>
    ValueType& Emplace(const KeyType& key, Args&&... args);

    /**
     * 특정 키에 대한 Entry 객체를 반환합니다.
     * @param key 검색 또는 삽입할 키
     * @return 키의 존재 여부에 따라 분기된 Entry 객체
     */
    [[nodiscard]] EntryType Entry(const KeyType& key);

    /**
     * Key에 해당하는 값을 찾습니다.
     * @param key 검색할 Key
     * @return 값에 대한 Optional 참조 (Key가 없으면 nullopt)
     */
    [[nodiscard]] Optional<ValueType&> Find(const KeyType& key);
    [[nodiscard]] Optional<const ValueType&> Find(const KeyType& key) const;

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
     * 특정 Key가 Map에 포함되어 있는지 확인합니다
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

    /** Map의 모든 키를 담은 Array를 생성하여 반환합니다. */
    [[nodiscard]] Array<KeyType> GetKeys() const;

    /** Map의 모든 값을 담은 Array를 생성하여 반환합니다. */
    [[nodiscard]] Array<ValueType> GetValues() const;

    void Swap(Map& other) noexcept;

public:
    /**
     * 키에 해당하는 값에 접근합니다. 키가 없으면 기본 생성된 값을 삽입 후 반환합니다.
     * @param key 접근할 키
     */
    [[nodiscard]] ValueType& operator[](const KeyType& key);

    [[nodiscard]] bool operator==(const Map&) const = default;
    [[nodiscard]] auto operator<=>(const Map&) const = default;

    // Iterator
    [[nodiscard]] IteratorType begin() noexcept;
    [[nodiscard]] IteratorType end() noexcept;
    [[nodiscard]] ConstIteratorType begin() const noexcept;
    [[nodiscard]] ConstIteratorType end() const noexcept;

    friend void swap(Map& lhs, Map& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    InternalMapType internal_map;
};
}

#include "SimpleEngine/Core/Container/Map.inl"
