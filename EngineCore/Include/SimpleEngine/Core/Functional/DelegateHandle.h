#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <atomic>
#include <utility>


namespace se
{
class ScopedDelegateHandle;

template <typename Signature>
class MultiDelegate;

template <typename Signature>
class SingleDelegate;

namespace detail
{
/**
 * 핸들이 시그니처를 몰라도 소유한 델리게이트와 상호작용 할 수 있게 하는 내부 인터페이스
 */
class IDelegateSlotOwner
{
public:
    /** 핸들의 구독을 해제합니다. 호출 중이면 표시만 하고 끝난 뒤 정리합니다. */
    virtual void RemoveSlot(ScopedDelegateHandle& handle) = 0;

    /** 슬롯이 가리키는 핸들 주소를 handle로 갱신합니다. */
    virtual void RelinkSlot(ScopedDelegateHandle& handle) = 0;

    /** 슬롯이 가리키는 핸들 주소만 지웁니다. */
    virtual void UnlinkSlot(ScopedDelegateHandle& handle) = 0;

protected:
    ~IDelegateSlotOwner() = default;
};
} // namespace detail

/**
 * 수동으로 관리하는 델리게이트 핸들
 */
struct DelegateHandle
{
    u64 id = 0;

    [[nodiscard]] bool IsValid() const noexcept { return id != 0; }
    void Invalidate() noexcept { id = 0; }

    [[nodiscard]] bool operator==(const DelegateHandle&) const noexcept = default;

    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

private:
    template <typename Signature>
    friend class MultiDelegate;

    template <typename Signature>
    friend class SingleDelegate;

    /** 델리게이트끼리 겹치지 않는 id를 만듭니다. */
    [[nodiscard]] static u64 GenerateNewId() noexcept
    {
        static std::atomic<u64> id_counter = 0;
        return ++id_counter;
    }
};

/**
 * 소멸하거나 Reset하면 구독을 해제하는 핸들입니다.
 * 델리게이트가 먼저 사라지면 비활성이 되고, 이후 소멸해도 아무 일도 하지 않습니다.
 */
class ScopedDelegateHandle
{
public:
    ScopedDelegateHandle() = default;

    ~ScopedDelegateHandle()
    {
        Reset();
    }

    ScopedDelegateHandle(const ScopedDelegateHandle&) = delete;
    ScopedDelegateHandle& operator=(const ScopedDelegateHandle&) = delete;

    ScopedDelegateHandle(ScopedDelegateHandle&& other) noexcept
    {
        *this = std::move(other);
    }

    ScopedDelegateHandle& operator=(ScopedDelegateHandle&& other) noexcept
    {
        if (this != &other)
        {
            Reset();

            owner = std::exchange(other.owner, nullptr);
            index = other.index;
            is_pending = other.is_pending;
            id = other.id;

            if (owner)
            {
                owner->RelinkSlot(*this);
            }
        }
        return *this;
    }

public:
    /** 구독을 해제합니다. 이미 해제됐거나 델리게이트가 사라졌으면 아무것도 하지 않습니다. */
    void Reset()
    {
        if (owner)
        {
            owner->RemoveSlot(*this);
        }
    }

    /**
     * 자동 해제를 끊고 수동 핸들을 반환합니다.
     * 반환값을 버리면 델리게이트가 살아 있는 동안 유지되는 영구 구독이 됩니다.
     */
    DelegateHandle Detach()
    {
        if (!owner)
        {
            return {};
        }

        owner->UnlinkSlot(*this);
        owner = nullptr;
        return DelegateHandle{ id };
    }

    /** 구독이 연결되어 있는지 확인합니다. */
    [[nodiscard]] bool IsValid() const noexcept { return owner != nullptr; }

private:
    template <typename Signature>
    friend class MultiDelegate;

    template <typename Signature>
    friend class SingleDelegate;

    ScopedDelegateHandle(detail::IDelegateSlotOwner& in_owner, usize in_index, bool in_is_pending, u64 in_id)
        : owner(&in_owner)
        , index(in_index)
        , is_pending(in_is_pending)
        , id(in_id)
    {
        owner->RelinkSlot(*this);
    }

private:
    detail::IDelegateSlotOwner* owner = nullptr;
    usize index = 0;
    bool is_pending = false;
    u64 id = 0;
};
} // namespace se
