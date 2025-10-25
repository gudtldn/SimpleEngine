#pragma once
#include <concepts>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"


template <typename T>
class [[nodiscard]] Optional
{
public:
    using InnerType = T;

public:
    Optional() noexcept = default;
    ~Optional() { Reset(); }

    template <typename U = T>
        requires std::constructible_from<T, U>
    Optional(std::optional<U>&& other_optional)
    {
        if (other_optional.has_value())
        {
            Construct(std::move(other_optional.value()));
        }
    }

    template <typename... Args>
    Optional(std::in_place_t, Args&&... args)
    {
        Construct(std::forward<Args>(args)...);
    }

    Optional(std::nullopt_t) noexcept
        : storage{}
    {
    }

    Optional(const T& in_value)
    {
        Construct(in_value);
    }

    Optional(T&& in_value)
    {
        Construct(std::move(in_value));
    }

    Optional(const Optional& other)
    {
        if (other.HasValue())
        {
            Construct(other.GetStoredValue());
        }
    }

    Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (other.HasValue())
        {
            Construct(std::move(other.GetStoredValue()));
            other.Reset();
        }
    }

    Optional& operator=(const T& in_value)
    {
        if (HasValue())
        {
            GetStoredValue() = in_value;
        }
        else
        {
            Construct(in_value);
        }
        return *this;
    }

    Optional& operator=(T&& in_value) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
    {
        if (HasValue())
        {
            GetStoredValue() = std::move(in_value);
        }
        else
        {
            Construct(std::move(in_value));
        }
        return *this;
    }

    Optional& operator=(const Optional& other)
    {
        if (this != &other)
        {
            Reset();
            if (other.HasValue())
            {
                Construct(other.GetStoredValue());
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            if (other.HasValue())
            {
                Construct(std::move(other.GetStoredValue()));
                other.Reset();
            }
        }
        return *this;
    }

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T& Value() &
    {
        if (!HasValue())
        {
            throw std::bad_optional_access{};
        }
        return GetStoredValue();
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] const T& Value() const &
    {
        if (!HasValue())
        {
            throw std::bad_optional_access{};
        }
        return GetStoredValue();
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T&& Value() &&
    {
        if (!HasValue())
        {
            throw std::bad_optional_access{};
        }
        return std::move(GetStoredValue());
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] const T&& Value() const &&
    {
        if (!HasValue())
        {
            throw std::bad_optional_access{};
        }
        return std::move(GetStoredValue());
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename U = std::remove_cv_t<T>>
        requires std::convertible_to<U, std::remove_cv_t<T>>
    [[nodiscard]] std::remove_cv_t<T> ValueOr(U&& default_value) const &
    {
        if (HasValue())
        {
            return GetStoredValue();
        }
        return static_cast<std::remove_cv_t<T>>(std::forward<U>(default_value));
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename U = std::remove_cv_t<T>>
        requires std::convertible_to<U, std::remove_cv_t<T>>
    [[nodiscard]] std::remove_cv_t<T> ValueOr(U&& default_value) &&
    {
        if (HasValue())
        {
            return GetStoredValue();
        }
        return static_cast<std::remove_cv_t<T>>(std::forward<U>(default_value));
    }

    /** 값이 존재할 때, fn(T) -> Optional<U>인 함수를 호출하여 새로운 Optional<U> 타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T&>, Optional>
    auto AndThen(Fn&& func) const &
    {
        using ResultT = std::invoke_result_t<Fn, const T&>;

        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), GetStoredValue());
        }
        return std::remove_cvref_t<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> Optional<U>인 함수를 호출하여 새로운 Optional<U> 타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, T>, Optional>
    auto AndThen(Fn&& func) &&
    {
        using ResultT = std::invoke_result_t<Fn, T>;

        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), std::move(GetStoredValue()));
        }
        return std::remove_cvref_t<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> Optional<U>인 함수를 호출하여 새로운 Optional<U> 타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T&&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T>, Optional>
    auto AndThen(Fn&& func) const &&
    {
        using ResultT = std::invoke_result_t<Fn, const T>;

        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), static_cast<const T&&>(GetStoredValue()));
        }
        return std::remove_cvref_t<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 U타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T&>>, std::nullopt_t, std::in_place_t>)
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
        && (!std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>)
    auto Transform(Fn&& func) &
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T&>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), GetStoredValue()) };
        }
        return Optional<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 U타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>, std::nullopt_t, std::in_place_t>)
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
        && (!std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>)
    auto Transform(Fn&& func) const &
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, const T&>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), GetStoredValue()) };
        }
        return Optional<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 U타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T>>, std::nullopt_t, std::in_place_t>)
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, T>>>
        && (!std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, T>>>)
    auto Transform(Fn&& func) &&
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), std::move(GetStoredValue())) };
        }
        return Optional<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 U타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<T, Fn, const T&&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T>>, std::nullopt_t, std::in_place_t>)
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, const T>>>
        && (!std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, const T>>>)
    auto Transform(Fn&& func) const &&
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, const T>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), std::move(GetStoredValue())) };
        }
        return Optional<ResultT>{};
    }

    /** 값이 없을 때, fn() -> Optional<T>인 함수를 호출하여, 새로운 Optional<T> 타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
        && std::copy_constructible<T>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional OrElse(Fn&& func) const &
    {
        if (HasValue())
        {
            return *this;
        }
        return std::forward<Fn>(func)();
    }

    /** 값이 없을 때, fn() -> Optional<T>인 함수를 호출하여, 새로운 Optional<T> 타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
        && std::move_constructible<T>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional OrElse(Fn&& func) &&
    {
        if (HasValue())
        {
            return std::move(*this);
        }
        return std::forward<Fn>(func)();
    }

    /** 새로운 값을 할당합니다. (내부에서 초기화) */
    template <typename... Args>
    T& Emplace(Args&&... args)
    {
        Reset();
        Construct(std::forward<Args>(args)...);
        return GetStoredValue();
    }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] bool HasValue() const noexcept { return is_value_set; }

    /** 가지고 있던 값을 삭제하고, nullopt로 변경합니다. */
    void Reset()
    {
        if (HasValue())
        {
            Destroy();
            is_value_set = false;
        }
    }

public:
    [[nodiscard]] T& operator*() { return Value(); }
    [[nodiscard]] const T& operator*() const { return Value(); }
    [[nodiscard]] T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] const T* operator->() const { return std::addressof(Value()); }

    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }
    [[nodiscard]] bool operator==(std::nullopt_t) const noexcept { return !HasValue(); }

    [[nodiscard]] bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return GetStoredValue() == other.GetStoredValue();
        }
        return HasValue() == other.HasValue();
    }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] bool operator==(const U& value) const
    {
        if (HasValue())
        {
            return GetStoredValue() == value;
        }
        return false;
    }

    [[nodiscard]] friend bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] friend bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    [[nodiscard]] T& GetStoredValue() &
    {
        return *std::launder(reinterpret_cast<T*>(&storage));
    }

    [[nodiscard]] const T& GetStoredValue() const &
    {
        return *std::launder(reinterpret_cast<const T*>(&storage));
    }

    [[nodiscard]] T&& GetStoredValue() &&
    {
        return std::move(*std::launder(reinterpret_cast<T*>(&storage)));
    }

    [[nodiscard]] const T&& GetStoredValue() const &&
    {
        return std::move(*std::launder(reinterpret_cast<const T*>(&storage)));
    }

    template <typename... Args>
    void Construct(Args&&... args)
    {
        new(storage) T(std::forward<Args>(args)...);
        is_value_set = true;
    }

    void Destroy()
    {
        std::destroy_at(std::addressof(GetStoredValue()));
    }

private:
    alignas(T) uint8 storage[sizeof(T)];
    bool is_value_set = false;
};

template <typename T>
class [[nodiscard]] Optional<T&>
{
public:
    using InnerType = T;

public:
    Optional() noexcept = default;
    ~Optional() = default;

    Optional(std::nullopt_t) noexcept
    {
    }

    Optional(T& in_value) noexcept
        : ref_ptr(std::addressof(in_value))
    {
    }

    Optional& operator=(std::nullopt_t) noexcept
    {
        ref_ptr = nullptr;
        return *this;
    }

    template <typename U>
        requires std::convertible_to<U*, T*>
    Optional(Optional<U>& other)
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    template <typename U>
        requires std::convertible_to<U*, T*>
    Optional(const Optional<U>& other)
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    Optional(const Optional&) = default;
    Optional& operator=(const Optional&) noexcept = default;
    Optional(Optional&&) noexcept = default;
    Optional& operator=(Optional&&) noexcept = default;

    // r-value 타입 T는 사용불가
    Optional(T&&) = delete;
    Optional& operator=(T&&) = delete;

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T& Value()
    {
        if (!HasValue())
        {
            throw std::bad_optional_access{};
        }
        return GetStoredValue();
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] const T& Value() const
    {
        if (!HasValue())
        {
            throw std::bad_optional_access{};
        }
        return GetStoredValue();
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] T& ValueOr(T& default_value)
    {
        if (HasValue())
        {
            return GetStoredValue();
        }
        return static_cast<T&>(default_value);
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] const T& ValueOr(const T& default_value) const
    {
        if (HasValue())
        {
            return GetStoredValue();
        }
        return default_value;
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T&>, Optional>
    auto AndThen(Fn&& func) const
    {
        using ResultT = std::invoke_result_t<Fn, const T&>;

        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), GetStoredValue());
        }
        return std::remove_cvref_t<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 U타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T&>>, std::nullopt_t, std::in_place_t>)
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
        && (!std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>)
    auto Transform(Fn&& func)
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T&>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), GetStoredValue()) };
        }
        return Optional<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 U타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>, std::nullopt_t, std::in_place_t>)
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
        && (!std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>)
    auto Transform(Fn&& func) const
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, const T&>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), GetStoredValue()) };
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn>
        && std::copy_constructible<T>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional OrElse(Fn&& func) const
    {
        if (HasValue())
        {
            return *this;
        }
        return std::forward<Fn>(func)();
    }

    /** 가지고 있던 값을 삭제하고, nullopt로 변경합니다. */
    void Reset() noexcept { ref_ptr = nullptr; }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] bool HasValue() const noexcept { return ref_ptr != nullptr; }

public:
    [[nodiscard]] T& operator*() { return Value(); }
    [[nodiscard]] const T& operator*() const { return Value(); }
    [[nodiscard]] T* operator->() const { return ref_ptr; }

    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    [[nodiscard]] bool operator==(std::nullopt_t) const noexcept { return !HasValue(); }

    [[nodiscard]] bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return GetStoredValue() == other.GetStoredValue();
        }
        return HasValue() == other.HasValue();
    }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] bool operator==(const U& value) const
    {
        if (HasValue())
        {
            return GetStoredValue() == value;
        }
        return false;
    }

    [[nodiscard]] friend bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] friend bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    T& GetStoredValue() { return *ref_ptr; }
    const T& GetStoredValue() const { return *ref_ptr; }

private:
    T* ref_ptr = nullptr;
};
