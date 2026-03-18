#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Graphics/Device/RID.h"

#include <concepts>
#include <limits>
#include <ranges>
#include <utility>


namespace se::graphics
{
/** SlotMap에 저장 가능한 타입 */
template <typename T>
concept SlotMapStorable = std::move_constructible<T> && std::default_initializable<T>;


/**
 * Generational Arena 기반의 SlotMap 컨테이너
 *
 * RID(index + generation)를 키로 사용하여 O(1) 접근을 제공합니다.
 * 삭제된 슬롯은 free list를 통해 재사용되며, generation이 증가하므로
 * 오래된 RID로의 접근은 자동으로 실패합니다.
 *
 * @tparam T 저장할 요소 타입
 */
template<SlotMapStorable T>
class SlotMap
{
    static constexpr uint32 FREE_SENTINEL = std::numeric_limits<uint32>::max();

    /** generation을 1 증가시키되, INVALID_GENERATION(0)은 건너뛰어 1로 wrap합니다. */
    [[nodiscard]] static constexpr uint32 NextGeneration(uint32 gen) noexcept
    {
        const uint32 next = gen + 1;
        return next == RID::INVALID_GENERATION ? 1 : next;
    }

    struct Slot
    {
        T data{};
        uint32 generation = RID::INVALID_GENERATION; // 0: 미사용 초기값(= INVALID). 활성 슬롯의 generation은 항상 1 이상

        // free list 연결: 사용 중이면 FREE_SENTINEL, 아니면 다음 free 슬롯 인덱스
        uint32 next_free = FREE_SENTINEL;
        bool occupied = false;
    };

public:
    SlotMap() = default;

    /**
     * 새 요소를 삽입하고 해당 요소를 가리키는 RID를 반환합니다.
     * free list에 빈 슬롯이 있으면 재사용하고, 없으면 새 슬롯을 할당합니다.
     */
    template <typename U = T>
        requires std::convertible_to<U&&, T>
    RID Insert(U&& value)
    {
        usize slot_index;

        if (free_head != FREE_SENTINEL)
        {
            // 기존 free_list 재사용
            slot_index = free_head;
            Slot& slot = slots[slot_index];
            free_head = slot.next_free;

            slot.data = std::forward<U>(value);
            slot.next_free = FREE_SENTINEL;
            slot.occupied = true;
        }
        else
        {
            // 빈 공간이 없을 경우, 새로 추가
            slot_index = slots.Len();
            slots.Push(Slot{
                .data = std::forward<U>(value),
                .generation = NextGeneration(0),
                .next_free = FREE_SENTINEL,
                .occupied = true,
            });
        }

        ++count;
        return {
            .index = static_cast<uint32>(slot_index),
            .generation = slots[slot_index].generation
        };
    }

    /**
     * RID에 해당하는 요소의 참조를 반환합니다.
     * RID가 무효하거나 세대가 불일치하면 NullOpt를 반환합니다.
     */
    template <typename Self>
    [[nodiscard]] auto Get(this Self& self, RID rid)
    {
        using OptRef = std::conditional_t<std::is_const_v<Self>, Optional<const T&>, Optional<T&>>;
        if (!self.IsValidRID(rid))
        {
            return OptRef{ NullOpt };
        }
        return OptRef{ self.slots[rid.index].data };
    }

    /**
     * RID에 해당하는 요소를 제거합니다.
     * 세대를 증가시켜 기존 RID를 무효화하고, 슬롯을 free list에 반환합니다.
     * @return 제거 성공 시 true, RID가 무효하면 false를 반환합니다.
     */
    bool Remove(RID rid)
    {
        if (!IsValidRID(rid))
        {
            return false;
        }

        Slot& slot = slots[rid.index];
        slot.data = T{};
        slot.occupied = false;
        slot.generation = NextGeneration(slot.generation);
        slot.next_free = free_head;
        free_head = rid.index;
        --count;

        return true;
    }

    /**
     * RID가 유효한지(슬롯이 존재하고, 세대가 일치하고, 사용 중인지) 검사합니다.
     */
    [[nodiscard]] bool IsValidRID(RID rid) const
    {
        if (std::cmp_greater_equal(rid.index, slots.Len()))  // rid.index >= slots.Len()
        {
            return false;
        }
        const Slot& slot = slots[rid.index];
        return slot.occupied && slot.generation == rid.generation;
    }

    /**
     * 모든 사용 중인 요소에 대해 콜백을 호출합니다.
     * @param fn void(RID, T&) 또는 void(RID, const T&) 시그니처의 callable입니다.
     */
    template <typename Self, typename Fn>
        requires std::invocable<Fn&, RID, std::conditional_t<std::is_const_v<Self>, const T&, T&>>
    void ForEach(this Self& self, Fn&& fn)
    {
        for (const auto [idx, slot] : self.slots | std::views::enumerate)
        {
            if (slot.occupied)
            {
                fn(RID{
                    .index = static_cast<uint32>(idx),
                    .generation = slot.generation,
                }, slot.data);
            }
        }
    }

    /** 현재 사용 중인 요소 수를 반환합니다. */
    [[nodiscard]] uint32 Count() const noexcept { return count; }

    /** SlotMap이 비어 있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept { return count == 0; }

    /** 모든 요소를 제거하고 내부 상태를 초기화합니다. */
    void Clear()
    {
        slots.Clear();
        free_head = FREE_SENTINEL;
        count = 0;
    }

private:
    Array<Slot> slots;
    uint32 free_head = FREE_SENTINEL;
    uint32 count = 0;
};
} // namespace se::graphics
