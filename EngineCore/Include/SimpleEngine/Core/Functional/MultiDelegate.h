#pragma once
#include <atomic>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "tracy/Tracy.hpp"


namespace se::core
{
struct DelegateHandle
{
    uint64 id = 0;

    [[nodiscard]] bool IsValid() const noexcept { return id != 0; }
    void Invalidate() noexcept { id = 0; }

    [[nodiscard]] bool operator==(const DelegateHandle&) const noexcept = default;
    [[nodiscard]] auto operator<=>(const DelegateHandle&) const noexcept = default;

    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

private:
    template <typename Signature>
    friend class MultiDelegate;

    static uint64 GenerateNewId()
    {
        static std::atomic<uint64> id_counter = 0;
        return ++id_counter;
    }

    static DelegateHandle CreateHandle()
    {
        return { GenerateNewId() };
    }
};

template <typename Signature>
class MultiDelegate;

template <typename R, typename... Args>
class MultiDelegate<R(Args...)>
{
    using FunctionType = Function<R(Args...)>;
    struct Binding
    {
        FunctionType callable;
        DelegateHandle handle;

        [[nodiscard]] bool operator==(const DelegateHandle& other_handle) const { return handle == other_handle; }
        [[nodiscard]] auto operator<=>(const Binding& other) const { return handle <=> other.handle; }
    };

public:
    MultiDelegate() = default;

public:
    template <typename Fn>
        requires (std::is_invocable_r_v<R, Fn, Args...> && !std::is_member_function_pointer_v<std::decay_t<Fn>>)
    DelegateHandle AddLambda(Fn&& func)
    {
        std::scoped_lock lock(mutex);

        DelegateHandle new_handle = DelegateHandle::CreateHandle();
        bindings.Emplace(std::forward<Fn>(func), new_handle);
        return new_handle;
    }

    bool Remove(DelegateHandle handle)
    {
        if (!handle.IsValid())
        {
            return false;
        }

        std::scoped_lock lock(mutex);
        const usize num_erased = bindings.RemoveIf([&handle](const Binding& binding)
        {
            return binding.handle == handle;
        });
        return num_erased > 0;
    }

    void Clear()
    {
        std::scoped_lock lock(mutex);
        bindings.Clear();
    }

    void Broadcast(Args... args) const
    {
        Array<Binding> bindings_copy;
        {
            std::scoped_lock lock(mutex);
            bindings_copy = bindings;
        }

        for (const auto& binding : bindings_copy)
        {
            binding.callable(args...);
        }
    }

private:
    mutable TracyLockable(std::mutex, mutex);
    Array<Binding> bindings;
};
}
