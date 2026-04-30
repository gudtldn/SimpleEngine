#pragma once
#include <tuple>
#include <utility>
#include <variant>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
template <typename MapType>
class MapEntry
{
private:
    friend MapType;

    using IteratorType = MapType::IteratorType;
    using KeyType = MapType::KeyType;
    using ValueType = MapType::ValueType;

public:
    /** 키가 존재하는 경우의 Entry */
    class OccupiedEntry
    {
        friend MapType;
        friend class MapEntry;

        OccupiedEntry(IteratorType it, MapType* ptr)
            : iter(it)
            , map_ptr(ptr)
        {
        }

    public:
        [[nodiscard]] const KeyType& Key() const noexcept { return iter->first; }
        [[nodiscard]] ValueType& Value() noexcept { return iter->second; }
        [[nodiscard]] const ValueType& Value() const noexcept { return iter->second; }

        /** 값을 덮어쓰고, 이전 값을 반환합니다. */
        [[nodiscard]] ValueType Set(const ValueType& new_value)
        {
            return Set(ValueType{ new_value });
        }

        /** 값을 덮어쓰고, 이전 값을 반환합니다. */
        [[nodiscard]] ValueType Set(ValueType&& new_value)
        {
            ValueType old_value = std::move(iter->second);
            iter->second = std::move(new_value);
            return old_value;
        }

        /** 해당 엔트리를 제거하고, 값의 소유권을 반환합니다. */
        [[nodiscard]] ValueType Remove()
        {
            ValueType value = std::move(iter->second);
            map_ptr->internal_map.erase(iter);
            return value;
        }

    private:
        IteratorType iter;
        MapType* map_ptr;
    };

    /** 키가 존재하지 않는 경우의 Entry */
    class VacantEntry
    {
        friend MapType;
        friend class MapEntry;

        VacantEntry(KeyType k, MapType* ptr)
            : key(std::move(k))
            , map_ptr(ptr)
        {
        }

    public:
        [[nodiscard]] const KeyType& Key() const noexcept { return key; }

        // 값을 삽입하고 새 값의 참조를 반환합니다. (OrInsert)
        [[nodiscard]] ValueType& OrInsert(const ValueType& value) { return EmplaceArgs(value); }
        [[nodiscard]] ValueType& OrInsert(ValueType&& value) { return EmplaceArgs(std::move(value)); }

        // 값을 인플레이스로 생성하여 삽입하고 새 값의 참조를 반환합니다.
        template <typename... Args>
        [[nodiscard]] ValueType& EmplaceArgs(Args&&... args)
        {
            auto [it, inserted] = map_ptr->internal_map.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(std::move(key)),
                std::forward_as_tuple(std::forward<Args>(args)...)
            );

            SE_ASSERT(inserted, "Entry API error: Key existed during VacantEmplace.");
            return it->second;
        }

    private:
        KeyType key;
        MapType* map_ptr;
    };

public:
    [[nodiscard]] bool IsOccupied() const noexcept { return storage.index() == 0; }
    [[nodiscard]] bool IsVacant() const noexcept { return storage.index() == 1; }

    [[nodiscard]] OccupiedEntry& AsOccupied()
    {
        SE_ASSERT(IsOccupied(), "Called AsOccupied() on a Vacant Entry.");
        return std::get<OccupiedEntry>(storage);
    }

    [[nodiscard]] VacantEntry& AsVacant()
    {
        SE_ASSERT(IsVacant(), "Called AsVacant() on an Occupied Entry.");
        return std::get<VacantEntry>(storage);
    }

    /** 키가 존재하는 경우, 해당 값을 반환합니다. (없다면 nullopt) */
    [[nodiscard]] Optional<ValueType&> GetValue()
    {
        if (IsOccupied())
        {
            return AsOccupied().Value();
        }
        return NullOpt;
    }

    /** 키가 존재하는 경우, 해당 값을 반환합니다. (없다면 nullopt) */
    [[nodiscard]] Optional<const ValueType&> GetValue() const
    {
        if (IsOccupied())
        {
            return AsOccupied().Value();
        }
        return NullOpt;
    }

    /** 키가 없으면 값을 삽입하고, 있으면 기존 값의 참조를 반환합니다. */
    ValueType& OrInsert(ValueType value)
    {
        if (IsOccupied())
        {
            return AsOccupied().Value();
        }
        return AsVacant().OrInsert(std::move(value));
    }

    /** 키가 없으면 람다를 실행하여 생성된 값을 삽입하고, 있으면 기존 값의 참조를 반환합니다. */
    template <typename Fn>
        requires std::is_invocable_r_v<ValueType, Fn>
    ValueType& OrInsertWith(Fn&& func)
    {
        if (IsOccupied())
        {
            return AsOccupied().Value();
        }
        return AsVacant().OrInsert(std::forward<Fn>(func)());
    }

    /** 키가 없으면 기본 생성자로 생성된 값을 삽입하고, 있으면 기존 값의 참조를 반환합니다. */
    ValueType& OrDefault() requires std::default_initializable<ValueType>
    {
        return OrInsert(ValueType{});
    }

    template <typename Fn>
        requires std::invocable<Fn, ValueType&>
    MapEntry& AndModify(Fn&& func)
    {
        if (IsOccupied())
        {
            func(AsOccupied().Value());
        }
        return *this;
    }

private:
    using EntryStorage = std::variant<OccupiedEntry, VacantEntry>;
    EntryStorage storage;

    explicit MapEntry(OccupiedEntry occ) : storage(std::move(occ)) {}
    explicit MapEntry(VacantEntry vac) : storage(std::move(vac)) {}

public:
    ~MapEntry() = default;

    MapEntry(const MapEntry&) = delete;
    MapEntry(MapEntry&&) noexcept = delete;
    MapEntry& operator=(const MapEntry&) = delete;
    MapEntry& operator=(MapEntry&&) noexcept = delete;
};
} // namespace se
