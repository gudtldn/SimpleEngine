// ReSharper disable CppOverrideWithDifferentVisibility
#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/DelegateHandle.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"

#include <thread>
#include <type_traits>


namespace se
{
template <typename Signature>
class SingleDelegate;

/**
 * 하나의 콜백을 담는 델리게이트
 * 생성한 스레드에서만 사용해야 합니다.
 */
template <typename R, typename... Args>
class SingleDelegate<R(Args...)> final : private detail::IDelegateSlotOwner
{
    using FunctionType = UniqueFunction<R(Args...)>;
    using ExecuteResult = std::conditional_t<std::is_void_v<R>, void, Optional<R>>;

    struct Slot
    {
        FunctionType callable;
        ScopedDelegateHandle* scoped = nullptr;
    };

public:
    SingleDelegate() = default;

    ~SingleDelegate()
    {
        SE_ASSERT(execute_depth == 0, "SingleDelegate destroyed during Execute");
        UnlinkAll();
    }

    SingleDelegate(const SingleDelegate&) = delete;
    SingleDelegate& operator=(const SingleDelegate&) = delete;

    SingleDelegate(SingleDelegate&& other) noexcept
    {
        *this = std::move(other);
    }

    SingleDelegate& operator=(SingleDelegate&& other) noexcept
    {
        SE_ASSERT(execute_depth == 0 && other.execute_depth == 0, "SingleDelegate moved during Execute");
        if (this != &other)
        {
            UnlinkAll();

            current = std::move(other.current);
            other.current = Slot{};
            is_current_cleared = std::exchange(other.is_current_cleared, false);

            if (current.scoped)
            {
                current.scoped->owner = this;
            }
        }
        return *this;
    }

public:
    /** 콜백을 바인딩합니다. 기존 바인딩은 교체되고, 기존 핸들은 비활성이 됩니다. */
    template <typename Fn>
        requires std::is_invocable_r_v<R, std::decay_t<Fn>&, Args...>
    [[nodiscard]] ScopedDelegateHandle Bind(Fn&& func)
    {
        CheckThread();
        UnlinkAll();

        const u64 id = DelegateHandle::GenerateNewId();
        Slot new_slot{ .callable = FunctionType{ std::forward<Fn>(func) }, .scoped = nullptr };

        // 실행 중인 콜백은 끝날 때까지 두고, 새 콜백은 대기 슬롯에 둡니다.
        if (execute_depth > 0)
        {
            is_current_cleared = true;
            pending = std::move(new_slot);
            return ScopedDelegateHandle{ *this, 0, true, id };
        }

        current = std::move(new_slot);
        is_current_cleared = false;
        return ScopedDelegateHandle{ *this, 0, false, id };
    }

    /** 바인딩을 해제합니다. 핸들은 비활성이 됩니다. */
    void Clear()
    {
        CheckThread();
        UnlinkAll();
        pending.Reset();
        ClearCurrent();
    }

    /** 바인딩이 있는지 확인합니다. */
    [[nodiscard]] bool IsBound() const
    {
        return pending.HasValue() || (!is_current_cleared && current.callable.IsValid());
    }

    /** 콜백을 호출합니다. 바인딩이 없으면 void는 아무것도 하지 않고, 그 외에는 빈 Optional을 반환합니다. */
    ExecuteResult Execute(Args... args)
    {
        CheckThread();

        if (is_current_cleared || !current.callable.IsValid())
        {
            if constexpr (std::is_void_v<R>)
            {
                return;
            }
            else
            {
                return NullOpt;
            }
        }

        ++execute_depth;
        if constexpr (std::is_void_v<R>)
        {
            current.callable(args...);
            FinishExecute();
            return;
        }
        else
        {
            ExecuteResult result{ current.callable(args...) };
            FinishExecute();
            return result;
        }
    }

private:
    virtual void RemoveSlot(ScopedDelegateHandle& handle) override
    {
        CheckThread();
        const bool is_pending = handle.is_pending;
        UnlinkScoped(SlotOf(handle));

        if (is_pending)
        {
            pending.Reset();
            return;
        }
        ClearCurrent();
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
        return handle.is_pending ? *pending : current;
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
        UnlinkScoped(current);
        if (pending)
        {
            UnlinkScoped(*pending);
        }
    }

    /** 실행 중인 콜백일 수 있으므로 Execute 중에는 표시만 합니다. */
    void ClearCurrent()
    {
        if (execute_depth > 0)
        {
            is_current_cleared = true;
            return;
        }
        current = Slot{};
        is_current_cleared = false;
    }

    void FinishExecute()
    {
        --execute_depth;
        if (execute_depth > 0)
        {
            return;
        }

        if (pending)
        {
            current = std::move(*pending);
            pending.Reset();
            is_current_cleared = false;
            if (current.scoped)
            {
                current.scoped->is_pending = false;
            }
            return;
        }

        if (is_current_cleared)
        {
            current = Slot{};
            is_current_cleared = false;
        }
    }

    void CheckThread() const
    {
#if SE_ENABLE_ASSERTS
        SE_ASSERT(std::this_thread::get_id() == owner_thread, "SingleDelegate used from a different thread");
#endif
    }

private:
    Slot current;
    Optional<Slot> pending; // Execute 중 바인딩된 콜백
    bool is_current_cleared = false;
    usize execute_depth = 0;

#if SE_ENABLE_ASSERTS
    std::thread::id owner_thread = std::this_thread::get_id();
#endif
};
} // namespace se
