#pragma once
#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <ranges>
#include <utility>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/MapEntry.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Utility/Debug.h"


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
    typename Allocator = DefaultAllocator<std::pair<const Key, Value>>
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

    template <std::input_iterator It, std::sentinel_for<It> Sent>
    Map(It first, Sent last);

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

    /**
     * 새로운 요소를 Map에 추가합니다.
     * 이미 동일한 키를 가진 요소가 존재할 경우, 해당 요소의 값을 대체합니다.
     * @param key 삽입 또는 업데이트할 키
     * @param value 키에 매핑할 값
     * @return 삽입되거나 업데이트된 값에 대한 참조
     */
    template <typename K = KeyType, typename V = ValueType>
        requires std::constructible_from<Key, K&&> && std::constructible_from<Value, V&&>
    ValueType& Insert(K&& key, V&& value);

    /**
     * 새로운 요소를 Map에 내부 생성(emplace)하여 추가합니다.
     * @tparam Args 요소의 생성자에 전달할 인수들의 타입
     * @param key 삽입할 키
     * @param args 요소의 생성자에 전달할 인수들
     */
    template <typename K = KeyType, typename... Args>
        requires std::constructible_from<Key, K&&> && std::constructible_from<Value, Args&&...>
    ValueType& Emplace(K&& key, Args&&... args);

    /**
     * 특정 키에 대한 Entry 객체를 반환합니다.
     * @param key 검색 또는 삽입할 키
     * @return 키의 존재 여부에 따라 분기된 Entry 객체
     */
    template <typename K = KeyType>
        requires std::constructible_from<Key, K&&>
    [[nodiscard]] EntryType Entry(K&& key);

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
     * @return 값에 대한 const 참조
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
    template <typename T = KeyType>
        requires std::constructible_from<T, const Key&>
    [[nodiscard]] Array<KeyType> Keys() const;

    /** Map의 모든 값을 담은 Array를 생성하여 반환합니다. */
    template <typename T = ValueType>
        requires std::constructible_from<T, const Value&>
    [[nodiscard]] Array<ValueType> Values() const;

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

template <typename Key, typename Value>
Archive& operator<<(Archive& ar, Map<Key, Value>& map)
{
    // TODO: 추후에 Array 대신 더 나은 방법으로 수정

    uint64 total_elements;
    if (ar.IsSaving())
    {
        total_elements = map.Len() * 2;
    }
    ar.BeginArray(total_elements);

    if (ar.IsLoading())
    {
        SE_ASSERT(total_elements % 2 == 0, "");
        const uint64 map_count = total_elements / 2;

        map.Clear();
        for (uint64 i = 0; i < map_count; ++i)
        {
            Key key;
            Value value;

            // 키와 값을 순서대로 읽음
            ar << key << value;
            map.Emplace(std::move(key), std::move(value));
        }
    }
    else
    {
        for (auto& [key, value] : map)
        {
            ar << const_cast<Key&>(key) << value;
        }
    }

    ar.EndArray();
    return ar;
}
}  // namespace se

#include "SimpleEngine/Core/Container/Map.inl"
