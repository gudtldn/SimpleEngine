#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

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
    template <typename> friend class Optional;

public:
    using ValueType = T;

public:
    constexpr Optional() noexcept = default;

    constexpr Optional(std::nullopt_t) noexcept
        : storage{}
    {
    }

    constexpr Optional(const T& in_value)
    {
        Construct(in_value);
    }

    constexpr Optional(T&& in_value)
    {
        Construct(std::move(in_value));
    }

    template <typename... Args>
    constexpr explicit Optional(std::in_place_t, Args&&... args)
    {
        Construct(std::forward<Args>(args)...);
    }

    // std::optional -> Optional 변환 생성자
    template <typename U>
        requires std::constructible_from<T, U>
    constexpr Optional(std::optional<U>&& other)
    {
        if (other.has_value())
        {
            Construct(std::move(other).value());
        }
    }

    // Optional<U> -> Optional<T> 변환 생성자
    template <typename U>
        requires std::constructible_from<T, const U&>
    constexpr Optional(const Optional<U>& other)
    {
        if (other.HasValue())
        {
            Construct(other.GetStoredValue());
        }
    }

    template <typename U>
        requires std::constructible_from<T, U&&>
    constexpr Optional(Optional<U>&& other)
    {
        if (other.HasValue())
        {
            Construct(std::move(other.GetStoredValue()));
            other.Reset();
        }
    }

    constexpr Optional(const Optional& other)
    {
        if (other.HasValue())
        {
            Construct(other.GetStoredValue());
        }
    }

    constexpr Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (other.HasValue())
        {
            Construct(std::move(other.GetStoredValue()));
            other.Reset();
        }
    }

    constexpr ~Optional()
    {
        Reset();
    }

    constexpr Optional& operator=(std::nullopt_t) noexcept
    {
        Reset();
        return *this;
    }

    constexpr Optional& operator=(const Optional& other)
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

    constexpr Optional& operator=(Optional&& other) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>)
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

    template <typename U>
        requires std::constructible_from<T, U>
        && std::assignable_from<T&, U>
        && se::details::NotOptionalOrInPlace<U>
    constexpr Optional& operator=(U&& value)
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
    [[nodiscard]] constexpr auto&& Value(this Self&& self)
    {
        SE_ASSERT(self.HasValue(), "Optional is empty!");
        return std::forward_like<Self>(self.GetStoredValue());
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename Self, typename U>
        requires std::convertible_to<U, T>
    [[nodiscard]] constexpr T ValueOr(this Self&& self, U&& default_value)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self).Value();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    /** 값이 있으면 반환하고, 없으면 T의 기본 생성 값을 반환합니다. */
    template <typename Self>
        requires std::default_initializable<T>
    [[nodiscard]] constexpr T ValueOrDefault(this Self&& self)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self).Value();
        }
        return T{};
    }

    /** 값이 존재할 때, fn(T) -> Optional<U>인 함수를 호출하여 새로운 Optional<U> 타입을 반환합니다. */
    template <typename Self, typename Fn>
        requires se::details::IsOptional<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>>
    constexpr auto AndThen(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>;
        if (self.HasValue())
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value());
        }
        return std::remove_cvref_t<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 Optional<U>를 반환합니다. */
    template <typename Self, typename Fn>
        requires (!se::traits::IsAnyOf<
                     std::remove_cv_t<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>>,
                     std::nullopt_t,
                     std::in_place_t>)
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>>>
    constexpr auto Transform(this Self&& self, Fn&& func)
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>>;
        if (self.HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value()) };
        }
        return Optional<ResultT>{};
    }

    /** 값이 없을 때, fn() -> Optional<T>인 함수를 호출하여 새로운 Optional<T>를 반환합니다. */
    template <typename Self, typename Fn>
        requires std::invocable<Fn>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    constexpr Optional OrElse(this Self&& self, Fn&& func)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self);
        }
        return std::invoke(std::forward<Fn>(func));
    }

    /** 새로운 값을 할당합니다. (내부에서 초기화) */
    template <typename... Args>
    constexpr T& Emplace(Args&&... args)
    {
        Reset();
        Construct(std::forward<Args>(args)...);
        return GetStoredValue();
    }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept { return is_value_set; }

    /** 가지고 있던 값을 삭제하고, nullopt로 변경합니다. */
    constexpr void Reset()
    {
        if (HasValue())
        {
            Destroy();
            is_value_set = false;
        }
    }

public:
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return HasValue(); }

    template <typename Self>
    [[nodiscard]] constexpr auto&& operator*(this Self&& self)
    {
        return std::forward<Self>(self).Value();
    }

    [[nodiscard]] constexpr T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] constexpr const T* operator->() const { return std::addressof(Value()); }

    [[nodiscard]] constexpr bool operator==(std::nullopt_t) const noexcept { return !HasValue(); }

    [[nodiscard]] constexpr bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return GetStoredValue() == other.GetStoredValue();
        }
        return HasValue() == other.HasValue();
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<T, U>)
    [[nodiscard]] constexpr bool operator==(const U& value) const
    {
        return HasValue() && GetStoredValue() == value;
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<U, T>)
    [[nodiscard]] friend constexpr bool operator==(const U& value, const Optional& other) { return other == value; }

    [[nodiscard]] friend constexpr bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

private:
    template <typename Self>
    [[nodiscard]] constexpr auto&& GetStoredValue(this Self&& self)
    {
        return std::forward_like<Self>(self.storage.value);
    }

    template <typename... Args>
    constexpr void Construct(Args&&... args)
    {
        std::construct_at(std::addressof(storage.value), std::forward<Args>(args)...);
        is_value_set = true;
    }

    constexpr void Destroy()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::destroy_at(std::addressof(storage.value));
        }
    }

private:
    union Storage
    {
        uint8 dummy;
        T value;

        constexpr Storage() : dummy{} {}
        constexpr ~Storage() {}
    };

    Storage storage{};
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

    constexpr Optional(T& in_value) noexcept
        : ref_ptr(std::addressof(in_value))
    {
    }

    // Value Optional -> Ref Optional 변환
    template <typename U>
        requires std::convertible_to<U*, T*>
    constexpr Optional(Optional<U>& other) noexcept
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    template <typename U>
        requires std::convertible_to<const U*, T*>
    constexpr Optional(const Optional<U>& other) noexcept
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    template <typename U>
    constexpr Optional(Optional<U>&&) = delete;

    // 다른 Optional<U&> 에서 변환 (예: Derived& -> Base&)
    template <typename U>
        requires std::convertible_to<U*, T*>
    constexpr Optional(const Optional<U&>& other) noexcept
    {
        if (other.HasValue())
        {
            ref_ptr = std::addressof(*other);
        }
    }

    ~Optional() = default;
    constexpr Optional(const Optional&) = default;
    constexpr Optional& operator=(const Optional&) = default;
    constexpr Optional(Optional&&) noexcept = default;
    constexpr Optional& operator=(Optional&&) noexcept = default;

    // r-value 타입 T는 사용불가
    constexpr Optional(T&&) noexcept = delete;
    constexpr Optional& operator=(T&&) noexcept = delete;

    constexpr Optional& operator=(std::nullopt_t) noexcept
    {
        Reset();
        return *this;
    }

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] constexpr T& Value() const
    {
        SE_ASSERT(HasValue(), "Optional is empty!");
        return *ref_ptr;
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] constexpr T& ValueOr(T& default_value) const
    {
        if (HasValue())
        {
            return *ref_ptr;
        }
        return default_value;
    }

    /** Optional에 저장된 값을 복사하여 Optional<T>로 반환합니다. */
    [[nodiscard]] constexpr Optional<std::remove_cv_t<T>> Copy() const
        requires std::copy_constructible<std::remove_cv_t<T>>
    {
        if (HasValue())
        {
            return Optional<std::remove_cv_t<T>>{ *ref_ptr };
        }
        return Optional<std::remove_cv_t<T>>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, T&>
        && se::details::IsOptional<std::invoke_result_t<Fn, T&>>
    constexpr auto AndThen(Fn&& func) const
    {
        using ResultT = std::invoke_result_t<Fn, T&>;
        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), *ref_ptr);
        }
        return std::remove_cvref_t<ResultT>{};
    }

    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 Optional<U>를 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&>
        && (!se::traits::IsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T&>>, std::nullopt_t, std::in_place_t>)
        && se::details::IsValidResultType<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
    constexpr auto Transform(Fn&& func) const
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T&>>;
        if (HasValue())
        {
            return Optional<ResultT>{ std::invoke(std::forward<Fn>(func), *ref_ptr) };
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    constexpr Optional OrElse(Fn&& func) const
    {
        if (HasValue())
        {
            return *this;
        }
        return std::invoke(std::forward<Fn>(func));
    }

    /** 가지고 있던 값을 삭제하고, nullopt로 변경합니다. */
    constexpr void Reset() noexcept { ref_ptr = nullptr; }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept { return ref_ptr != nullptr; }

public:
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return HasValue(); }

    [[nodiscard]] constexpr T& operator*() const { return Value(); }
    [[nodiscard]] constexpr T* operator->() const { return ref_ptr; }

    [[nodiscard]] constexpr bool operator==(std::nullopt_t) const noexcept { return !HasValue(); }
    [[nodiscard]] friend constexpr bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

    [[nodiscard]] constexpr bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return *ref_ptr == *other.ref_ptr;
        }
        return HasValue() == other.HasValue();
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<T, U>)
    [[nodiscard]] constexpr bool operator==(const U& value) const
    {
        return HasValue() && *ref_ptr == value;
    }

    template <typename U>
        requires (!se::details::IsOptional<std::remove_cvref_t<U>> && std::equality_comparable_with<U, T>)
    [[nodiscard]] friend constexpr bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    T* ref_ptr = nullptr;
};
