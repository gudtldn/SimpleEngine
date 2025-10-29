#pragma once
#include <functional>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <unordered_map>
#include <utility>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/MapEntry.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"


namespace se
{
/**
 * 해시 테이블 기반의 Key-Value 컨테이너
 *
 * @tparam Key 키의 타입
 * @tparam Value 값의 타입
 * @tparam Hasher 해시 함수의 타입
 * @tparam KeyEq 키 비교 함수의 타입
 * @tparam Allocator 메모리 할당자 타입
 */
template <
    typename Key,
    typename Value,
    typename Hasher = std::hash<Key>,
    typename KeyEq = std::equal_to<Key>,
    typename Allocator = core::memory::DefaultAllocator<std::pair<const Key, Value>>
>
class HashMap
{
private:
    friend class MapEntry<HashMap>;

    using InternalMapType = std::unordered_map<Key, Value, Hasher, KeyEq, Allocator>;
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

    using EntryType = MapEntry<HashMap>;

public:
    HashMap() = default;
    explicit HashMap(SizeType capacity);
    HashMap(std::initializer_list<PairType> init_list);

    template <std::input_iterator It>
    HashMap(It first, It last);

    ~HashMap() = default;
    HashMap(const HashMap&) = default;
    HashMap& operator=(const HashMap&) = default;
    HashMap(HashMap&&) noexcept = default;
    HashMap& operator=(HashMap&&) noexcept = default;

public:
    template <std::ranges::input_range Rng>
    [[nodiscard]] static HashMap FromRange(Rng&& range);

public:
    /** Map에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** Map이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** 재할당 없이 Map이 담을 수 있는 요소의 수를 반환합니다. (내부 버킷 수) */
    [[nodiscard]] SizeType Capacity() const noexcept;

    /** 최소 new_capacity 만큼의 요소를 저장할 수 있도록 용량을 예약합니다. */
    void Reserve(SizeType new_capacity);

    /** Map의 모든 요소를 제거합니다. */
    void Clear() noexcept;

    /**
     * 새로운 요소를 Map에 내부 생성(emplace)하여 추가합니다.
     * @tparam Args 요소의 생성자에 전달할 인수들의 타입
     * @param key 삽입할 키
     * @param args 요소의 생성자에 전달할 인수들
     */
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

    void Swap(HashMap& other) noexcept;

public:
    /**
     * 키에 해당하는 값에 접근합니다. 키가 없으면 기본 생성된 값을 삽입 후 반환합니다.
     * @param key 접근할 키
     */
    [[nodiscard]] ValueType& operator[](const KeyType& key);

    // Iterator
    [[nodiscard]] IteratorType begin() noexcept;
    [[nodiscard]] IteratorType end() noexcept;
    [[nodiscard]] ConstIteratorType begin() const noexcept;
    [[nodiscard]] ConstIteratorType end() const noexcept;

    friend void swap(HashMap& lhs, HashMap& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    InternalMapType internal_map;
};
}

#include "SimpleEngine/Core/Container/HashMap.inl"
