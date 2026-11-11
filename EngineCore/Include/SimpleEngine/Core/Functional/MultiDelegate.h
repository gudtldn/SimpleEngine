// ReSharper disable CppOverrideWithDifferentVisibility
#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/DelegateHandle.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <algorithm>
#include <ranges>
#include <thread>


namespace se
{
template <typename Signature>
class MultiDelegate
{
    static_assert(traits::AlwaysFalse<Signature>, "MultiDelegate only supports signatures returning void");
};

/**
 * 여러 구독자에게 알리는 델리게이트
 * 생성한 스레드에서만 사용해야 하며, 구독자간 호출 순서는 보장하지 않습니다.
 */
template <typename... Args>
class MultiDelegate<void(Args...)> final : private detail::IDelegateSlotOwner
{
    using FunctionType = UniqueFunction<void(Args...)>;

    struct Slot
    {
        FunctionType callable;
        ScopedDelegateHandle* scoped = nullptr;
        u64 id = 0;
        bool is_removed = false;
    };

public:
    MultiDelegate() = default;

    ~MultiDelegate()
    {
        SE_ASSERT(broadcast_depth == 0, "MultiDelegate destroyed during Broadcast");
        UnlinkAll();
    }

    MultiDelegate(const MultiDelegate&) = delete;
    MultiDelegate& operator=(const MultiDelegate&) = delete;

    MultiDelegate(MultiDelegate&& other) noexcept
    {
        *this = std::move(other);
    }

    MultiDelegate& operator=(MultiDelegate&& other) noexcept
    {
        SE_ASSERT(broadcast_depth == 0 && other.broadcast_depth == 0, "MultiDelegate moved during Broadcast");
        if (this != &other)
        {
            UnlinkAll();

            slots = std::move(other.slots);
            other.slots.Clear();

            for (Slot& slot : slots)
            {
                if (slot.scoped)
                {
                    slot.scoped->owner = this;
                }
            }
        }
        return *this;
    }

public:
    /** 구독을 추가합니다. 반환된 핸들이 소멸하면 구독이 해제됩니다. */
    template <typename Fn>
        requires std::is_invocable_r_v<void, std::decay_t<Fn>&, Args...>
    [[nodiscard]] ScopedDelegateHandle Add(Fn&& func)
    {
        CheckThread();

        const bool is_pending = broadcast_depth > 0;
        Array<Slot>& target = is_pending ? pending_slots : slots;
        const u64 id = DelegateHandle::GenerateNewId();
        target.Emplace(Slot{
            .callable = FunctionType{ std::forward<Fn>(func) },
            .scoped = nullptr,
            .id = id,
            .is_removed = false,
        });
        return ScopedDelegateHandle{ *this, target.Len() - 1, is_pending, id };
    }

    /** Detach한 핸들로 구독을 해제합니다. */
    bool Remove(DelegateHandle handle)
    {
        CheckThread();
        if (!handle.IsValid())
        {
            return false;
        }

        if (const auto index = FindLiveSlot(slots, handle.id))
        {
            RemoveAt(false, *index);
            return true;
        }
        if (const auto index = FindLiveSlot(pending_slots, handle.id))
        {
            RemoveAt(true, *index);
            return true;
        }
        return false;
    }

    /** 모든 구독을 해제합니다. */
    void Clear()
    {
        CheckThread();
        UnlinkAll();

        if (broadcast_depth > 0)
        {
            for (Slot& slot : slots)
            {
                slot.is_removed = true;
            }
            for (Slot& slot : pending_slots)
            {
                slot.is_removed = true;
            }
            has_removed_slot = true;
            return;
        }

        slots.Clear();
    }

    /** 해제되지 않은 구독이 하나라도 있는지 확인합니다. */
    [[nodiscard]] bool IsBound() const
    {
        const auto is_live = [](const Slot& slot) { return !slot.is_removed; };
        return std::ranges::any_of(slots, is_live) || std::ranges::any_of(pending_slots, is_live);
    }

    /** 모든 구독자를 호출합니다. */
    void Broadcast(Args... args)
    {
        CheckThread();

        // 순회 중에는 slots의 크기와 원소 위치가 바뀌지 않습니다. Add는 대기 배열로 가고, 제거는 표시만 합니다.
        ++broadcast_depth;
        for (usize i = 0; i < slots.Len(); ++i)
        {
            if (!slots[i].is_removed)
            {
                slots[i].callable(args...);
            }
        }
        --broadcast_depth;

        if (broadcast_depth == 0)
        {
            FlushPending();
        }
    }

private:
    virtual void RemoveSlot(ScopedDelegateHandle& handle) override
    {
        CheckThread();
        RemoveAt(handle.is_pending, handle.index);
    }

    virtual void RelinkSlot(ScopedDelegateHandle& handle) override
    {
        SlotOf(handle).scoped = &handle;
    }

    virtual void UnlinkSlot(ScopedDelegateHandle& handle) override
    {
        SlotOf(handle).scoped = nullptr;
    }

    [[nodiscard]] Slot& SlotOf(const ScopedDelegateHandle& handle)
    {
        return (handle.is_pending ? pending_slots : slots)[handle.index];
    }

    [[nodiscard]] static Optional<usize> FindLiveSlot(const Array<Slot>& target, u64 id)
    {
        for (const auto [index, slot] : target | std::views::enumerate)
        {
            if (slot.id == id && !slot.is_removed)
            {
                return static_cast<usize>(index);
            }
        }
        return NullOpt;
    }

    static void UnlinkScoped(Slot& slot)
    {
        if (slot.scoped)
        {
            slot.scoped->owner = nullptr;
            slot.scoped = nullptr;
        }
    }

    void UnlinkAll()
    {
        for (Slot& slot : slots)
        {
            UnlinkScoped(slot);
        }
        for (Slot& slot : pending_slots)
        {
            UnlinkScoped(slot);
        }
    }

    void RemoveAt(bool is_pending, usize index)
    {
        Array<Slot>& target = is_pending ? pending_slots : slots;
        UnlinkScoped(target[index]);

        // 실행 중인 콜백일 수 있으므로 Broadcast 중에는 표시만 합니다.
        if (broadcast_depth > 0)
        {
            target[index].is_removed = true;
            has_removed_slot = true;
            return;
        }

        RemoveAtSwap(target, index);
    }

    /** 마지막 슬롯을 index로 옮겨 지우고, 옮긴 슬롯의 핸들 인덱스를 갱신합니다. */
    static void RemoveAtSwap(Array<Slot>& target, usize index)
    {
        target.RemoveAtSwap(index);
        if (index < target.Len() && target[index].scoped)
        {
            target[index].scoped->index = index;
        }
    }

    void FlushPending()
    {
        if (has_removed_slot)
        {
            // 뒤에서부터 지우면 당겨 온 슬롯은 이미 검사를 마친 살아 있는 슬롯입니다.
            for (usize i = slots.Len(); i-- > 0;)
            {
                if (slots[i].is_removed)
                {
                    RemoveAtSwap(slots, i);
                }
            }
            has_removed_slot = false;
        }

        for (Slot& slot : pending_slots)
        {
            if (slot.is_removed)
            {
                continue;
            }

            Slot& moved = slots.Emplace(std::move(slot));
            if (moved.scoped)
            {
                moved.scoped->index = slots.Len() - 1;
                moved.scoped->is_pending = false;
            }
        }
        pending_slots.Clear();
    }

    void CheckThread() const
    {
#if SE_ENABLE_ASSERTS
        SE_ASSERT(std::this_thread::get_id() == owner_thread, "MultiDelegate used from a different thread");
#endif
    }

private:
    Array<Slot> slots;
    Array<Slot> pending_slots; // Broadcast 중 추가된 구독
    usize broadcast_depth = 0;
    bool has_removed_slot = false;

#if SE_ENABLE_ASSERTS
    std::thread::id owner_thread = std::this_thread::get_id();
#endif
};
} // namespace se
