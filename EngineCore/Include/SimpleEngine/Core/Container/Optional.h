#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/ContainerTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>


namespace se
{
struct NullOptType {};
constexpr NullOptType NullOpt;

/**
 * 값의 존재 여부를 포함하는 래퍼 클래스
 * @tparam T 관리할 값의 타입
 */
template <typename T>
class Optional
{
    template <typename> friend class Optional;

public:
    using ValueType = T;

public:
    constexpr Optional() = default;
    constexpr Optional(NullOptType) noexcept {}

    constexpr Optional(const T& in_value) { Create(in_value); }
    constexpr Optional(T&& in_value) noexcept { Create(std::move(in_value)); }

    // std::optional -> Optional 변환 생성자
    template <typename U>
        requires std::constructible_from<T, U>
    constexpr Optional(std::optional<U>&& other)
    {
        if (other.has_value())
        {
            Create(std::move(other).value());
        }
    }

    // Optional<U> -> Optional<T> 변환 생성자
    template <typename U>
        requires std::constructible_from<T, const U&>
    constexpr Optional(const Optional<U>& other)
    {
        if (other.HasValue())
        {
            Create(other.Value());
        }
    }

    template <typename U>
        requires std::constructible_from<T, U&&>
    constexpr Optional(Optional<U>&& other)
    {
        if (other.HasValue())
        {
            Create(std::move(other).Value());
            other.Reset();
        }
    }

    constexpr Optional(const Optional& other)
    {
        if (other.HasValue())
        {
            Create(other.Value());
        }
    }

    constexpr Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (other.HasValue())
        {
            Create(std::move(other.Value()));
            other.Reset();
        }
    }

    constexpr ~Optional() requires (std::is_trivially_destructible_v<T>) = default;

    constexpr ~Optional() requires (!std::is_trivially_destructible_v<T>)
    {
        Reset();
    }

    constexpr Optional& operator=(NullOptType) noexcept
    {
        Reset();
        return *this;
    }

    template <typename U>
    requires std::constructible_from<T, U&&>
        && std::assignable_from<T&, U&&>
        && (!traits::OptionalLike<U>)
    constexpr Optional& operator=(U&& value)
    {
        if (HasValue())
        {
            Value() = std::forward<U>(value);
        }
        else
        {
            Create(std::forward<U>(value));
        }
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
                    Value() = other.Value();
                }
                else
                {
                    Create(other.Value());
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
                    Value() = std::move(other).Value();
                }
                else
                {
                    Create(std::move(other).Value());
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

public:
    /** 새로운 값을 할당합니다. (내부에서 초기화) */
    template <typename... Args>
        requires std::constructible_from<T, Args&&...>
    constexpr T& Emplace(Args&&... args)
    {
        Reset();
        Create(std::forward<Args>(args)...);
        return storage.value;
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Value(this Self&& self)
    {
        SE_ASSERT(self.HasValue(), "Optional is empty!");
        return std::forward_like<Self>(self.storage.value);
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename Self, typename U>
        requires std::constructible_from<T, U&&>
    [[nodiscard]] constexpr T ValueOr(this Self&& self, U&& default_value)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self).Value();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 T의 기본 생성 값을 반환합니다. */
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

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const
    {
        return is_value_set;
    }

    /** 가지고 있던 값을 삭제하고, NullOpt로 변경합니다. */
    constexpr void Reset()
    {
        if (HasValue())
        {
            Destroy();
        }
    }

public:
    /** 값이 존재할 때, fn(T) -> U인 함수를 호출하여 새로운 Optional<U>를 반환합니다. */
    template <typename Self, typename Fn>
        requires std::invocable<Fn, decltype(std::declval<Self>().Value())>
        && (!traits::IsAnyOfDecayed<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>, NullOptType>)
        && (!std::is_void_v<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>>)
    [[nodiscard]] constexpr auto Map(this Self&& self, Fn&& func)
    {
        using InvokeResult = std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>;

        if (self.HasValue())
        {
            return Optional<InvokeResult>{
                std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value())
            };
        }
        return Optional<InvokeResult>{};
    }

    /** 값이 존재할 때, fn(T) -> Optional<U>인 함수를 호출하여 새로운 Optional<U> 타입을 반환합니다. */
    template <typename Self, typename Fn>
        requires std::invocable<Fn, decltype(std::declval<Self>().Value())>
        && traits::OptionalLike<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>>
    constexpr auto AndThen(this Self&& self, Fn&& func)
    {
        using InvokeResult = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>;
        if (self.HasValue())
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value());
        }
        return std::remove_cvref_t<InvokeResult>{};
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

    /** 값이 존재할 때, fn(const T&)을 호출해 부수 효과를 실행하고 자기 자신을 그대로 반환합니다. */
    template <typename Self, typename Fn>
        requires std::invocable<Fn, const T&>
    constexpr auto&& Inspect(this Self&& self, Fn&& func)
    {
        if (self.HasValue())
        {
            std::invoke(std::forward<Fn>(func), *std::as_const(self));
        }
        return std::forward<Self>(self);
    }

public:
    [[nodiscard]] constexpr explicit operator bool() const
    {
        return HasValue();
    }

    template <typename Self>
    [[nodiscard]] constexpr auto&& operator*(this Self&& self)
    {
        return std::forward<Self>(self).Value();
    }

    [[nodiscard]] constexpr T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] constexpr const T* operator->() const { return std::addressof(Value()); }

    [[nodiscard]] constexpr bool operator==(NullOptType) const noexcept { return !HasValue(); }
    [[nodiscard]] friend constexpr bool operator==(NullOptType, const Optional& other) noexcept { return !other; }

    [[nodiscard]] constexpr bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return Value() == other.Value();
        }
        return HasValue() == other.HasValue();
    }

    template <typename U>
        requires (!traits::OptionalLike<U> && std::equality_comparable_with<T, U>)
    [[nodiscard]] constexpr bool operator==(const U& value) const
    {
        return HasValue() && Value() == value;
    }

private:
    template <typename... Args>
    constexpr void Create(Args&&... args)
    {
        std::construct_at(const_cast<std::remove_const_t<T>*>(std::addressof(storage.value)), std::forward<Args>(args)...);
        is_value_set = true;
    }

    constexpr void Destroy()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            std::destroy_at(const_cast<std::remove_const_t<T>*>(std::addressof(storage.value)));
        }
        is_value_set = false;
    }

private:
    union Storage
    {
        u8 dummy;
        T value;

        constexpr Storage() : dummy{} {}
        constexpr ~Storage() {}
    };

    Storage storage{};
    bool is_value_set = false;
};

/** T*에 대한 Null Pointer Optimization(NPO) 특수화 */
template <typename T>
class Optional<T*>
{
    template <typename> friend class Optional;

public:
    using ValueType = T*;

public:
    constexpr Optional() = default;
    constexpr Optional(NullOptType) noexcept {}
    constexpr Optional(T* in_value) : value_ptr(in_value) {}

    // std::optional -> Optional 변환 생성자
    template <typename U>
        requires std::convertible_to<U*, T*>
    constexpr Optional(std::optional<U*>&& other)
    {
        if (other.has_value())
        {
            value_ptr = other.value();
        }
    }

    // 다른 포인터 타입 변환 (Derived* -> Base*)
    template <typename U>
        requires std::convertible_to<U*, T*>
    constexpr Optional(const Optional<U*>& other) noexcept
    {
        if (other.HasValue())
        {
            value_ptr = other.value_ptr;
        }
    }

    ~Optional() = default;
    constexpr Optional(const Optional&) = default;
    constexpr Optional(Optional&&) noexcept = default;

    constexpr Optional& operator=(NullOptType) noexcept
    {
        Reset();
        return *this;
    }

    template <typename U>
        requires std::convertible_to<U*, T*> && (!traits::OptionalLike<U>)
    constexpr Optional& operator=(U* value)
    {
        value_ptr = value;
        return *this;
    }

    constexpr Optional& operator=(const Optional& other) = default;
    constexpr Optional& operator=(Optional&& other) noexcept = default;

public:
    template <typename... Args>
        requires std::constructible_from<T*, Args&&...>
    constexpr T* Emplace(Args&&... args) = delete;

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] constexpr T* Value() const
    {
        SE_ASSERT(HasValue(), "Optional is empty!");
        return value_ptr;
    }

    /** Optional이 가지고 있는 포인터를 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename U>
        requires std::constructible_from<T*, U&&>
    [[nodiscard]] constexpr T* ValueOr(U&& default_value) const
    {
        if (HasValue())
        {
            return value_ptr;
        }
        return static_cast<T*>(std::forward<U>(default_value));
    }

    /** Optional이 가지고 있는 포인터를 반환하거나, 값이 없으면 nullptr를 반환합니다. */
    [[nodiscard]] constexpr T* ValueOrDefault() const
    {
        if (HasValue())
        {
            return value_ptr;
        }
        return nullptr;
    }

    /** 포인터가 nullptr이 아닌지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept
    {
        return value_ptr != nullptr;
    }

    /** 내부 포인터를 nullptr로 초기화하여 빈 상태로 만듭니다. */
    constexpr void Reset() noexcept
    {
        value_ptr = nullptr;
    }

public:
    /** 값이 존재할 때, fn(T*) -> U를 호출하여 새로운 Optional을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T*>
        && (!traits::IsAnyOfDecayed<std::invoke_result_t<Fn, T*>, NullOptType>)
        && (!std::is_void_v<std::invoke_result_t<Fn, T*>>)
    [[nodiscard]] constexpr auto Map(Fn&& func) const
    {
        if (HasValue())
        {
            return Optional<std::invoke_result_t<Fn, T*>>{ std::invoke(std::forward<Fn>(func), value_ptr) };
        }
        return Optional<std::invoke_result_t<Fn, T*>>{};
    }

    /** 값이 존재할 때, fn(T*) -> Optional<U>를 호출합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T*>
        && traits::OptionalLike<std::invoke_result_t<Fn, T*>>
    [[nodiscard]] constexpr auto AndThen(Fn&& func) const
    {
        using InvokeResult = std::invoke_result_t<Fn, T*>;
        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), value_ptr);
        }
        return std::remove_cvref_t<InvokeResult>{};
    }

    /** 값이 없을 때, fn() -> Optional<T*> 호출합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    [[nodiscard]] constexpr Optional OrElse(Fn&& func) const
    {
        if (HasValue())
        {
            return *this;
        }
        return std::invoke(std::forward<Fn>(func));
    }

    /** 값이 존재할 때, fn(const T*)을 호출해 부수 효과를 실행하고 자기 자신을 그대로 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T*>
    [[nodiscard]] constexpr const Optional& Inspect(Fn&& func) const
    {
        if (HasValue())
        {
            std::invoke(std::forward<Fn>(func), static_cast<const T*>(value_ptr));
        }
        return *this;
    }

public:
    [[nodiscard]] constexpr explicit operator bool() const { return HasValue(); }

    [[nodiscard]] constexpr T* const& operator*() const { return value_ptr; }
    [[nodiscard]] constexpr T*& operator*() { return value_ptr; }

    [[nodiscard]] constexpr T* const* operator->() const { return &value_ptr; }
    [[nodiscard]] constexpr T** operator->() { return &value_ptr; }

    [[nodiscard]] constexpr bool operator==(NullOptType) const noexcept { return !HasValue(); }
    [[nodiscard]] friend constexpr bool operator==(NullOptType, const Optional& other) noexcept { return !other; }

    [[nodiscard]] constexpr bool operator==(const Optional& other) const
    {
        return value_ptr == other.value_ptr;
    }

    template <typename U>
        requires (!traits::OptionalLike<U> && std::equality_comparable_with<T*, U>)
    [[nodiscard]] constexpr bool operator==(const U& value) const
    {
        return HasValue() && value_ptr == value;
    }

    template <typename U>
        requires (!traits::OptionalLike<U> && std::equality_comparable_with<U, T*>)
    [[nodiscard]] friend constexpr bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    T* value_ptr = nullptr;
};

/** T&에 대한 Optional 특수화 */
template <typename T>
class Optional<T&>
{
    template <typename> friend class Optional;

public:
    using ValueType = T&;

public:
    constexpr Optional() = default;
    constexpr Optional(NullOptType) noexcept {}
    constexpr Optional(T& in_value) : value_ptr(std::addressof(in_value)) {}

    // r-value 타입 T는 사용불가 (Dangling 방지)
    constexpr Optional(T&&) = delete;
    constexpr Optional& operator=(T&&) noexcept = delete;

    // Value Optional -> Ref Optional 변환
    template <typename U>
        requires std::convertible_to<U*, T*>
    constexpr Optional(Optional<U>& other) noexcept
    {
        if (other.HasValue())
        {
            value_ptr = std::addressof(*other);
        }
    }

    // const Optional<U> -> Optional<const U&> 변환
    template <typename U>
        requires std::convertible_to<const U*, T*>
    constexpr Optional(const Optional<U>& other) noexcept
    {
        if (other.HasValue())
        {
            value_ptr = std::addressof(*other);
        }
    }

    // 다른 Optional<U&> 에서 변환 (예: Derived& -> Base&)
    template <typename U>
        requires std::convertible_to<U*, T*>
    constexpr Optional(const Optional<U&>& other) noexcept
    {
        if (other.HasValue())
        {
            value_ptr = std::addressof(*other);
        }
    }

    ~Optional() = default;
    constexpr Optional(const Optional&) = default;
    constexpr Optional& operator=(const Optional&) = default;
    constexpr Optional(Optional&&) noexcept = default;
    constexpr Optional& operator=(Optional&&) noexcept = default;

    constexpr Optional& operator=(NullOptType) noexcept
    {
        Reset();
        return *this;
    }

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] constexpr T& Value() const
    {
        SE_ASSERT(HasValue(), "Optional is empty!");
        return *value_ptr;
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] constexpr T& ValueOr(T& default_value) const
    {
        if (HasValue())
        {
            return *value_ptr;
        }
        return default_value;
    }

    [[nodiscard]] constexpr T& ValueOr(T&&) const = delete; // TODO: C++26 delete description 적기
    // = delete("Rvalues are not allowed to avoid dangling references");

    /** Optional에 저장된 값을 복사하여 Optional<T>로 반환합니다. */
    [[nodiscard]] constexpr Optional<std::remove_cv_t<T>> Copy() const
        requires std::copy_constructible<std::remove_cv_t<T>>
    {
        if (HasValue())
        {
            return Optional<std::remove_cv_t<T>>{ *value_ptr };
        }
        return Optional<std::remove_cv_t<T>>{};
    }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept { return value_ptr != nullptr; }

    /** 가지고 있던 값을 삭제하고, NullOpt로 변경합니다. */
    constexpr void Reset() noexcept { value_ptr = nullptr; }

public:
    /** 값이 존재할 때, fn(T&) -> U를 호출하여 새로운 Optional을 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&>
        && (!traits::IsAnyOfDecayed<std::invoke_result_t<Fn, T&>, NullOptType>)
        && (!std::is_void_v<std::invoke_result_t<Fn, T&>>)
    [[nodiscard]] constexpr auto Map(Fn&& func) const
    {
        if (HasValue())
        {
            return Optional<std::invoke_result_t<Fn, T&>>{
                std::invoke(std::forward<Fn>(func), *value_ptr)
            };
        }
        return Optional<std::invoke_result_t<Fn, T&>>{};
    }

    /** 값이 존재할 때, fn(T&) -> Optional<U>를 호출합니다. */
    template <typename Fn>
        requires std::invocable<Fn, T&>
        && traits::OptionalLike<std::invoke_result_t<Fn, T&>>
    [[nodiscard]] constexpr auto AndThen(Fn&& func) const
    {
        using InvokeResult = std::invoke_result_t<Fn, T&>;
        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func), *value_ptr);
        }
        return std::remove_cvref_t<InvokeResult>{};
    }

    /** 값이 없을 때, fn() -> Optional<T>를 호출합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    [[nodiscard]] constexpr Optional OrElse(Fn&& func) const
    {
        if (HasValue())
        {
            return *this;
        }
        return std::invoke(std::forward<Fn>(func));
    }

    /** 값이 존재할 때, fn(const T&)을 호출해 부수 효과를 실행하고 자기 자신을 그대로 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn, const T&>
    [[nodiscard]] constexpr const Optional& Inspect(Fn&& func) const
    {
        if (HasValue())
        {
            std::invoke(std::forward<Fn>(func), std::as_const(*value_ptr));
        }
        return *this;
    }

public:
    [[nodiscard]] constexpr explicit operator bool() const { return HasValue(); }

    [[nodiscard]] constexpr T* operator->() const { return value_ptr; }
    [[nodiscard]] constexpr T& operator*() const { return *value_ptr; }

    [[nodiscard]] constexpr bool operator==(NullOptType) const noexcept { return !HasValue(); }
    [[nodiscard]] friend constexpr bool operator==(NullOptType, const Optional& other) noexcept { return !other; }

    [[nodiscard]] constexpr bool operator==(const Optional& other) const
    {
        if (HasValue() && other.HasValue())
        {
            return *value_ptr == *other.value_ptr;
        }
        return HasValue() == other.HasValue();
    }

    template <typename U>
        requires (!se::traits::OptionalLike<U> && std::equality_comparable_with<T, U>)
    [[nodiscard]] constexpr bool operator==(const U& value) const
    {
        return HasValue() && *value_ptr == value;
    }

    template <typename U>
        requires (!se::traits::OptionalLike<U> && std::equality_comparable_with<U, T>)
    [[nodiscard]] friend constexpr bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    T* value_ptr = nullptr;
};
} // namespace se
