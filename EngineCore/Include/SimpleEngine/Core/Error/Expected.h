#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>


namespace se
{
// Forward declaration
template <typename E>
class Unexpected;


/**
 * 성공(T) 또는 실패(E) 중 하나의 값을 보유하는 래퍼 클래스.
 * 항상 둘 중 하나의 유효한 상태를 가집니다. (Uninitialized 상태 없음)
 *
 * @tparam T 성공 시 보유하는 값의 타입
 * @tparam E 실패 시 보유하는 에러의 타입
 */
template <typename T, typename E>
class [[nodiscard]] Expected
{
    template <typename, typename>
    friend class Expected;

public:
    using ValueType = T;
    using ErrorType = E;
    using UnexpectedType = Unexpected<E>;

public:
    /** T가 기본 생성 가능할 때, Ok(T{}) 상태로 생성합니다. */
    constexpr Expected()
        requires std::default_initializable<T>
    {
        CreateValue();
    }

    /** Ok(value) 상태로 생성합니다. */
    template <typename U = T>
        requires (!std::same_as<std::remove_cvref_t<U>, Expected>)
        && std::constructible_from<T, U>
    constexpr Expected(U&& value)
    {
        CreateValue(std::forward<U>(value));
    }

    /** Err(error) 상태로 생성합니다. */
    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(const Unexpected<Err>& error)
    {
        CreateError(error.Error());
    }

    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(Unexpected<Err>&& error)
    {
        CreateError(std::move(error.Error()));
    }

    constexpr Expected(const Expected& other)
    {
        if (other.has_value)
        {
            CreateValue(other.storage.value);
        }
        else
        {
            CreateError(other.storage.error);
        }
    }

    constexpr Expected(Expected&& other)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>)
    {
        if (other.has_value)
        {
            CreateValue(std::move(other.storage.value));
        }
        else
        {
            CreateError(std::move(other.storage.error));
        }
    }

    constexpr ~Expected() requires (std::is_trivially_destructible_v<T> && std::is_trivially_destructible_v<E>) = default;

    constexpr ~Expected() requires (!std::is_trivially_destructible_v<T> || !std::is_trivially_destructible_v<E>)
    {
        Destroy();
    }

    constexpr Expected& operator=(const Expected& other)
    {
        if (this != &other)
        {
            if (other.has_value)
            {
                if (has_value)
                {
                    storage.value = other.storage.value;
                }
                else
                {
                    Destroy();
                    CreateValue(other.storage.value);
                }
            }
            else
            {
                if (!has_value)
                {
                    storage.error = other.storage.error;
                }
                else
                {
                    Destroy();
                    CreateError(other.storage.error);
                }
            }
        }
        return *this;
    }

    constexpr Expected& operator=(Expected&& other)
        noexcept(
            std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>
            && std::is_nothrow_move_assignable_v<E> && std::is_nothrow_move_constructible_v<E>
        )
    {
        if (this != &other)
        {
            if (other.has_value)
            {
                if (has_value)
                {
                    storage.value = std::move(other.storage.value);
                }
                else
                {
                    Destroy();
                    CreateValue(std::move(other.storage.value));
                }
            }
            else
            {
                if (!has_value)
                {
                    storage.error = std::move(other.storage.error);
                }
                else
                {
                    Destroy();
                    CreateError(std::move(other.storage.error));
                }
            }
        }
        return *this;
    }

public:
    /** 새로운 값으로 Ok 상태를 직접 생성(in-place)합니다. */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T& Emplace(Args&&... args)
    {
        Destroy();
        CreateValue(std::forward<Args>(args)...);
        return storage.value;
    }

public:
    /** Ok 상태인지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept { return has_value; }

    /** Err 상태인지 확인합니다. */
    [[nodiscard]] constexpr bool HasError() const noexcept { return !has_value; }

    /** 보유한 값을 반환합니다. Ok 상태가 아니면 assert로 실패합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Value(this Self&& self)
    {
        SE_ASSERT(self.has_value, "Attempted to access value of an Expected that contains an error.");
        return std::forward_like<Self>(self.storage.value);
    }

    /** 보유한 에러를 반환합니다. Err 상태가 아니면 assert로 실패합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Error(this Self&& self)
    {
        SE_ASSERT(!self.has_value, "Attempted to access error of an Expected that contains a value.");
        return std::forward_like<Self>(self.storage.error);
    }

    /** 보유한 값을 반환하거나, Err 상태이면 default_value를 반환합니다. */
    template <typename Self, typename U>
        requires std::convertible_to<U&&, T>
    [[nodiscard]] constexpr T ValueOr(this Self&& self, U&& default_value)
    {
        if (self.has_value)
        {
            return std::forward<Self>(self).Value();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    /** 보유한 값을 반환하거나, Err 상태이면 T의 기본 생성 값을 반환합니다. */
    template <typename Self>
        requires std::default_initializable<T>
    [[nodiscard]] constexpr T ValueOrDefault(this Self&& self)
    {
        if (self.has_value)
        {
            return std::forward<Self>(self).Value();
        }
        return T{};
    }

    /** Ok 상태일 때, fn(T) -> Expected<U, E>를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Self, typename Fn>
        requires se::traits::IsSpecializationOf<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>, Expected>
    [[nodiscard]] constexpr auto AndThen(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>;
        if (self.has_value)
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value());
        }
        return ResultT{ Unexpected(std::forward<Self>(self).Error()) };
    }

    /** Ok 상태일 때, fn(T) -> U를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Self, typename Fn>
    [[nodiscard]] constexpr auto Map(this Self&& self, Fn&& func)
    {
        using ResultU = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>;
        using ResultT = Expected<ResultU, E>;
        if (self.has_value)
        {
            return ResultT{ std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value()) };
        }
        return ResultT{ Unexpected(std::forward<Self>(self).Error()) };
    }

    /** Err 상태일 때, fn(E) -> Expected<T, F>를 호출하여 새로운 Expected<T, F>를 반환합니다. */
    template <typename Self, typename Fn>
        requires se::traits::IsSpecializationOf<std::invoke_result_t<Fn, decltype(std::declval<Self>().Error())>, Expected>
    [[nodiscard]] constexpr auto OrElse(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        if (!self.has_value)
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error());
        }
        return ResultT{ std::forward<Self>(self).Value() };
    }

    /** Err 상태일 때, fn(E) -> F를 호출하여 새로운 Expected<T, F>를 반환합니다. */
    template <typename Self, typename Fn>
    [[nodiscard]] constexpr auto MapError(this Self&& self, Fn&& func)
    {
        using NewErrorType = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        using ResultT = Expected<T, NewErrorType>;
        if (!self.has_value)
        {
            return ResultT{ Unexpected(std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error())) };
        }
        return ResultT{ std::forward<Self>(self).Value() };
    }

public:
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value; }

    template <typename Self>
    [[nodiscard]] constexpr auto&& operator*(this Self&& self) { return std::forward<Self>(self).Value(); }

    [[nodiscard]] constexpr T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] constexpr const T* operator->() const { return std::addressof(Value()); }

private:
    template <typename... Args>
    constexpr void CreateValue(Args&&... args)
    {
        std::construct_at(std::addressof(storage.value), std::forward<Args>(args)...);
        has_value = true;
    }

    template <typename... Args>
    constexpr void CreateError(Args&&... args)
    {
        std::construct_at(std::addressof(storage.error), std::forward<Args>(args)...);
        has_value = false;
    }

    constexpr void Destroy()
    {
        if (has_value)
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                std::destroy_at(std::addressof(storage.value));
            }
        }
        else
        {
            if constexpr (!std::is_trivially_destructible_v<E>)
            {
                std::destroy_at(std::addressof(storage.error));
            }
        }
    }

private:
    union Storage
    {
        uint8 dummy;
        T value;
        E error;

        constexpr Storage() : dummy{} {}
        constexpr ~Storage() {}
    };

    Storage storage{};
    bool has_value = false;
};


/**
 * 성공(void) 또는 실패(E) 중 하나의 상태를 보유하는 Expected의 void 특수화.
 * 항상 둘 중 하나의 유효한 상태를 가집니다. (Uninitialized 상태 없음)
 *
 * @tparam E 실패 시 보유하는 에러의 타입
 */
template <typename E>
class [[nodiscard]] Expected<void, E>
{
    template <typename, typename>
    friend class Expected;

public:
    using ValueType = void;
    using ErrorType = E;
    using UnexpectedType = Unexpected<E>;

public:
    /** Ok 상태로 생성합니다. */
    constexpr Expected() = default;

    /** Err 상태로 생성합니다. */
    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(const Unexpected<Err>& error)
    {
        CreateError(error.Error());
    }

    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(Unexpected<Err>&& error)
    {
        CreateError(std::move(error.Error()));
    }

    constexpr Expected(const Expected& other)
    {
        if (!other.has_value)
        {
            CreateError(other.storage.error);
        }
    }

    constexpr Expected(Expected&& other)
        noexcept(std::is_nothrow_move_constructible_v<E>)
    {
        if (!other.has_value)
        {
            CreateError(std::move(other.storage.error));
        }
    }

    constexpr ~Expected() requires (std::is_trivially_destructible_v<E>) = default;

    constexpr ~Expected() requires (!std::is_trivially_destructible_v<E>)
    {
        Destroy();
    }

    constexpr Expected& operator=(const Expected& other)
    {
        if (this != &other)
        {
            if (!other.has_value)
            {
                if (!has_value)
                {
                    storage.error = other.storage.error;
                }
                else
                {
                    CreateError(other.storage.error);
                }
            }
            else
            {
                Destroy();
            }
        }
        return *this;
    }

    constexpr Expected& operator=(Expected&& other)
        noexcept(std::is_nothrow_move_assignable_v<E> && std::is_nothrow_move_constructible_v<E>)
    {
        if (this != &other)
        {
            if (!other.has_value)
            {
                if (!has_value)
                {
                    storage.error = std::move(other.storage.error);
                }
                else
                {
                    CreateError(std::move(other.storage.error));
                }
            }
            else
            {
                Destroy();
            }
        }
        return *this;
    }

public:
    /** Ok 상태로 되돌립니다. */
    constexpr void Emplace()
    {
        Destroy();
    }

public:
    /** Ok 상태인지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept { return has_value; }

    /** Err 상태인지 확인합니다. */
    [[nodiscard]] constexpr bool HasError() const noexcept { return !has_value; }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value; }

    /** Ok 상태인지 확인합니다. Err 상태이면 assert로 실패합니다. */
    constexpr void Value() const
    {
        SE_ASSERT(has_value, "Attempted to access value of an Expected<void, E> that contains an error.");
    }

    /** 보유한 에러를 반환합니다. Err 상태가 아니면 assert로 실패합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Error(this Self&& self)
    {
        SE_ASSERT(!self.has_value, "Attempted to access error of a successful Expected<void, E>.");
        return std::forward_like<Self>(self.storage.error);
    }

    /** Ok 상태일 때, fn() -> Expected<U, E>를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn>, Expected>
    [[nodiscard]] constexpr auto AndThen(Fn&& func) const
    {
        using ResultT = std::invoke_result_t<Fn>;
        if (has_value)
        {
            return std::invoke(std::forward<Fn>(func));
        }
        return ResultT{ Unexpected(Error()) };
    }

    /** Ok 상태일 때, fn() -> U를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
    [[nodiscard]] constexpr auto Map(Fn&& func) const
    {
        using ResultU = std::invoke_result_t<Fn>;
        using ResultT = Expected<ResultU, E>;
        if (has_value)
        {
            if constexpr (std::is_void_v<ResultU>)
            {
                std::invoke(std::forward<Fn>(func));
                return ResultT{};
            }
            else
            {
                return ResultT{ std::invoke(std::forward<Fn>(func)) };
            }
        }
        return ResultT{ Unexpected(Error()) };
    }

    /** Err 상태일 때, fn(E) -> Expected<void, F>를 호출하여 새로운 Expected<void, F>를 반환합니다. */
    template <typename Self, typename Fn>
        requires se::traits::IsSpecializationOf<std::invoke_result_t<Fn, decltype(std::declval<Self>().Error())>, Expected>
    [[nodiscard]] constexpr auto OrElse(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        if (!self.has_value)
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error());
        }
        return ResultT{};
    }

    /** Err 상태일 때, fn(E) -> F를 호출하여 새로운 Expected<void, F>를 반환합니다. */
    template <typename Self, typename Fn>
    [[nodiscard]] constexpr auto MapError(this Self&& self, Fn&& func)
    {
        using NewErrorType = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        using ResultT = Expected<void, NewErrorType>;
        if (!self.has_value)
        {
            return ResultT{ Unexpected(std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error())) };
        }
        return ResultT{};
    }

private:
    template <typename... Args>
    constexpr void CreateError(Args&&... args)
    {
        std::construct_at(std::addressof(storage.error), std::forward<Args>(args)...);
        has_value = false;
    }

    constexpr void Destroy()
    {
        if (!has_value)
        {
            if constexpr (!std::is_trivially_destructible_v<E>)
            {
                std::destroy_at(std::addressof(storage.error));
            }
            has_value = true;
        }
    }

private:
    union Storage
    {
        uint8 dummy;
        E error;

        constexpr Storage() : dummy{} {}
        constexpr ~Storage() {}
    };

    Storage storage{};
    bool has_value = true;
};


/**
 * 함수의 실패를 나타내는 래퍼 클래스.
 * Expected<T, E>에 에러 값을 전달하기 위해 사용됩니다.
 */
template <typename E>
class Unexpected
{
public:
    using ErrorType = E;

public:
    template <typename Err = E>
        requires (!std::same_as<std::remove_cvref_t<Err>, Unexpected>)
        && std::constructible_from<E, Err>
    constexpr explicit Unexpected(Err&& error)
        : error_value(std::forward<Err>(error))
    {
    }

    template <typename... Args>
        requires std::constructible_from<ErrorType, Args...>
    constexpr explicit Unexpected(Args&&... args)
        : error_value(std::forward<Args>(args)...)
    {
    }

    constexpr Unexpected(const Unexpected&) = default;
    constexpr Unexpected(Unexpected&&) noexcept = default;
    constexpr Unexpected& operator=(const Unexpected&) = default;
    constexpr Unexpected& operator=(Unexpected&&) noexcept = default;

public:
    /** 저장된 에러 값을 반환합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Error(this Self&& self) noexcept
    {
        return std::forward_like<Self>(self.error_value);
    }

private:
    E error_value;
};

template <typename E>
Unexpected(E) -> Unexpected<E>;
} // namespace se
