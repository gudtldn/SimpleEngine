#pragma once

#include <cassert>
#include <concepts>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"

// forward declaration
template <typename T>
class Optional;

namespace se::details
{
    template <typename T>
    concept IsOptional = traits::IsSpecializationOf<T, Optional>;

    template <typename T>
    concept NotOptionalOrInPlace =
        !IsOptional<std::remove_cvref_t<T>>
        && !std::same_as<std::remove_cvref_t<T>, std::in_place_t>
        && !std::same_as<std::remove_cvref_t<T>, std::nullopt_t>;

    template <typename T>
    concept IsValidResultType = !std::is_array_v<T> && std::is_object_v<T>;
} // namespace se::details

template <typename T>
class Optional
{
public:
    using ValueType = T;

public:
    constexpr Optional() noexcept = default;

    constexpr Optional(std::nullopt_t) noexcept
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

    template <typename... Args>
    explicit Optional(std::in_place_t, Args&&... args)
    {
        Construct(std::forward<Args>(args)...);
    }

    template <typename U = T>
        requires std::constructible_from<T, U>
    Optional(std::optional<U>&& other)
    {
        if (other.has_value())
        {
            Construct(std::move(other).value());
        }
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

    ~Optional()
    {
        Reset();
    }

    Optional& operator=(std::nullopt_t) noexcept
    {
        Reset();
        return *this;
    }

    Optional& operator=(const Optional& other)
    {
        if (this != &other)
        {
            if (other.HasValue())
            {
                if (HasValue())
                {
                    GetStoredValue() = other.GetStoredValue();
                }
                else
                {
                    Construct(other.GetStoredValue());
                }
            }
            else
            {
                Reset();
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>)
    {
        if (this != &other)
        {
            if (other.HasValue())
            {
                if (HasValue())
                {
                    GetStoredValue() = std::move(other.GetStoredValue());
                }
                else
                {
                    Construct(std::move(other.GetStoredValue()));
                }
                other.Reset();
            }
            else
            {
                Reset();
            }
        }
        return *this;
    }

    template <typename U = T>
        requires std::constructible_from<T, U>
        && std::assignable_from<T&, U>
        && se::details::NotOptionalOrInPlace<U>
    Optional& operator=(U&& value)
    {
        if (HasValue())
        {
            GetStoredValue() = std::forward<U>(value);
        }
        else
        {
            Construct(std::forward<U>(value));
        }
        return *this;
    }

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    template <typename Self>
    [[nodiscard]] auto&& Value(this Self&& self)
    {
        assert(self.HasValue() && "Optional is empty!");
        return std::forward_like<Self>(self.GetStoredValue());
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename Self, typename U>
        requires std::convertible_to<U, T>
    [[nodiscard]] T ValueOr(this Self&& self, U&& default_value)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self).Value();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    /** 값이 있으면 반환하고, 없으면 T의 기본 생성 값을 반환합니다. (Lazy Evaluation) */
    template <typename Self>
        requires std::default_initializable<T>
    [[nodiscard]] T ValueOrDefault(this Self&& self)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self).Value();
        }
        return T{};
    }

    /** 값이 존재할 때, fn(T) -> Optional<U>인 함수를 호출하여 새로운 Optional<U> 타입을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && se::details::IsOptional<std::invoke_result_t<Fn, const T&>>
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
        && se::details::IsOptional<std::invoke_result_t<Fn, T>>
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
        && se::details::IsOptional<std::invoke_result_t<Fn, const T>>
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
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
    auto Map(Fn&& func) &
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
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
    auto Map(Fn&& func) const &
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
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, T>>>
    auto Map(Fn&& func) &&
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
        requires std::invocable<Fn, const T&&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T>>, std::nullopt_t, std::in_place_t>)
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, const T>>>
    auto Map(Fn&& func) const &&
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
    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    template <typename Self>
    [[nodiscard]] decltype(auto) operator*(this Self&& self)
    {
        return std::forward_like<Self>(self.Value());
    }

    [[nodiscard]] T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] const T* operator->() const { return std::addressof(Value()); }

    [[nodiscard]] bool operator==(std::nullopt_t) const noexcept { return !HasValue(); }

    [[nodiscard]] bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return GetStoredValue() == other.GetStoredValue();
        }
        return HasValue() == other.HasValue();
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<T, U>)
    [[nodiscard]] bool operator==(const U& value) const
    {
        return HasValue() && GetStoredValue() == value;
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<U, T>)
    [[nodiscard]] friend bool operator==(const U& value, const Optional& other) { return other == value; }

    [[nodiscard]] friend bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

private:
    template <typename Self>
    [[nodiscard]] decltype(auto) GetStoredValue(this Self&& self)
    {
        using CastType = se::traits::DeduceRetType<Self, T*>;
        return *std::launder(reinterpret_cast<CastType>(&std::forward<Self>(self).storage));
    }

    template <typename... Args>
    void Construct(Args&&... args)
    {
        ::new(storage) T(std::forward<Args>(args)...);
        is_value_set = true;
    }

    void Destroy()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::destroy_at(std::addressof(GetStoredValue()));
        }
    }

private:
    alignas(T) uint8 storage[sizeof(T)];
    bool is_value_set = false;
};

template <typename T>
class Optional<T&>
{
public:
    using ValueType = T;

public:
    constexpr Optional() noexcept = default;
    constexpr Optional(std::nullopt_t) noexcept {}

    Optional(T& in_value) noexcept
        : ref_ptr(std::addressof(in_value))
    {
    }

    // Value Optional -> Ref Optional 변환
    template <typename U>
        requires std::convertible_to<U*, T*>
    Optional(Optional<U>& other) noexcept
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    template <typename U>
        requires std::convertible_to<const U*, T*>
    Optional(const Optional<U>& other) noexcept
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    template <typename U>
    Optional(Optional<U>&&) = delete;

    // 다른 Optional<U&> 에서 변환 (예: Derived& -> Base&)
    template <typename U>
        requires std::convertible_to<U*, T*>
    Optional(const Optional<U&>& other) noexcept
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    ~Optional() = default;
    Optional(const Optional&) = default;
    Optional& operator=(const Optional&) = default;
    Optional(Optional&&) noexcept = default;
    Optional& operator=(Optional&&) noexcept = default;

    // r-value 타입 T는 사용불가
    Optional(T&&) noexcept = delete;
    Optional& operator=(T&&) noexcept = delete;

    Optional& operator=(std::nullopt_t) noexcept
    {
        Reset();
        return *this;
    }

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T& Value() const
    {
        assert(HasValue() && "Optional is empty!");
        return GetStoredValue();
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] T& ValueOr(T& default_value) const
    {
        if (HasValue())
        {
            return GetStoredValue();
        }
        return default_value;
    }

    template <typename Fn>
        requires std::invocable<Fn, T&>
        && se::details::IsOptional<std::invoke_result_t<Fn, T&>>
    auto AndThen(Fn&& func) const
    {
        using ResultT = std::invoke_result_t<Fn, T&>;

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
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
    auto Map(Fn&& func) const
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T&>>;

        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), GetStoredValue()) };
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional OrElse(Fn&& func) const
    {
        if (HasValue())
        {
            return *this;
        }
        return std::invoke(std::forward<Fn>(func));
    }

    /** 가지고 있던 값을 삭제하고, nullopt로 변경합니다. */
    void Reset() noexcept { ref_ptr = nullptr; }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] bool HasValue() const noexcept { return ref_ptr != nullptr; }

public:
    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    [[nodiscard]] T& operator*() const { return Value(); }
    [[nodiscard]] T* operator->() const { return ref_ptr; }

    [[nodiscard]] bool operator==(std::nullopt_t) const noexcept { return !HasValue(); }
    [[nodiscard]] friend bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

    [[nodiscard]] bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return GetStoredValue() == other.GetStoredValue();
        }
        return HasValue() == other.HasValue();
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<T, U>)
    [[nodiscard]] bool operator==(const U& value) const
    {
        return HasValue() && GetStoredValue() == value;
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<U, T>)
    [[nodiscard]] friend bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    T& GetStoredValue() const { return *ref_ptr; }

private:
    T* ref_ptr = nullptr;
};
