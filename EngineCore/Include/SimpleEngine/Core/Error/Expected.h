#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
// Forward declaration
template <typename E>
class Unexpected;


template <typename T, typename E>
class [[nodiscard]] Expected
{
public:
    using ValueType = T;
    using ErrorType = E;
    using UnexpectedType = Unexpected<E>;

public:
    /** Uninitialized 상태로 Expected를 생성합니다. */
    constexpr Expected()
        : storage(std::in_place_index<0>, std::monostate{})
    {
    }

    /** 값이 있는 Expected를 생성합니다. */
    template <typename U = T>
        requires(!std::same_as<std::remove_cvref_t<U>, Expected>)
        && std::constructible_from<T, U>
    constexpr Expected(U&& value)
        : storage(std::in_place_index<1>, std::forward<U>(value))
    {
    }

    /** 에러가 있는 Expected를 생성합니다. */
    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(const Unexpected<Err>& error)
        : storage(std::in_place_index<2>, error.Error())
    {
    }

    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(Unexpected<Err>&& error)
        : storage(std::in_place_index<2>, std::move(error.Error()))
    {
    }

    /** 값을 내부 생성하여 Expected를 생성합니다. */
    template <typename... Args>
    constexpr Expected(std::in_place_t, Args&&... args)
        : storage(std::in_place_index<1>, std::forward<Args>(args)...)
    {
    }

    constexpr Expected(const Expected&) = default;
    constexpr Expected(Expected&&) noexcept = default;
    constexpr Expected& operator=(const Expected&) = default;
    constexpr Expected& operator=(Expected&&) noexcept = default;

public:
    /** 내부 저장소에 새로운 값을 직접 생성(in-place)합니다. */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T& Emplace(Args&&... args)
    {
        return storage.template emplace<1>(std::forward<Args>(args)...);
    }

public:
    /** Expected가 값을 가지고 있는지 확인합니다. */
    [[nodiscard]] constexpr bool HasValue() const noexcept { return storage.index() == 1; }

    /** Expected가 에러를 가지고 있는지 확인합니다. */
    [[nodiscard]] constexpr bool HasError() const noexcept { return storage.index() == 2; }

    /** 가지고 있는 값을 반환합니다. 값이 없으면 assert로 실패합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Value(this Self&& self)
    {
        SE_ASSERT(self.HasValue(), "Attempted to access value of an Expected that contains an error.");
        return std::forward_like<Self>(std::get<1>(self.storage));
    }

    /** 가지고 있는 에러를 반환합니다. 에러가 없으면 assert로 실패합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Error(this Self&& self)
    {
        SE_ASSERT(self.HasError(), "Attempted to access error of an Expected that contains a value.");
        return std::forward_like<Self>(std::get<2>(self.storage));
    }

    /** 가지고 있는 값을 반환하거나, 에러가 있으면 default_value를 반환합니다. */
    template <typename Self, typename U>
        requires std::convertible_to<U&&, T>
    [[nodiscard]] constexpr T ValueOr(this Self&& self, U&& default_value)
    {
        if (self.HasValue())
        {
            return std::forward<Self>(self).Value();
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    /** 값이 존재할 때, fn(T) -> Expected<U, E> 함수를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Self, typename Fn>
        requires se::traits::IsSpecializationOf<std::invoke_result_t<Fn, decltype(std::declval<Self>().Value())>, Expected>
    constexpr auto AndThen(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>;
        if (self.HasValue())
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value());
        }
        return ResultT{ Unexpected(std::forward<Self>(self).Error()) };
    }

    /** 값이 존재할 때, fn(T) -> U 함수를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Self, typename Fn>
    constexpr auto Map(this Self&& self, Fn&& func)
    {
        using ResultU = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Value())>;
        using ResultT = Expected<ResultU, E>;
        if (self.HasValue())
        {
            return ResultT{ std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Value()) };
        }
        return ResultT{ Unexpected(std::forward<Self>(self).Error()) };
    }

    /** 에러가 존재할 때, fn(E) -> Expected<T, F> 함수를 호출하여 새로운 Expected<T, F>를 반환합니다. */
    template <typename Self, typename Fn>
        requires se::traits::IsSpecializationOf<std::invoke_result_t<Fn, decltype(std::declval<Self>().Error())>, Expected>
    constexpr auto OrElse(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        if (self.HasError())
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error());
        }
        return ResultT{ std::forward<Self>(self).Value() };
    }

    /** 에러가 존재할 때, fn(E) -> F 함수를 호출하여 새로운 Expected<T, F>를 반환합니다. */
    template <typename Self, typename Fn>
    constexpr auto MapError(this Self&& self, Fn&& func)
    {
        using NewErrorType = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        using ResultT = Expected<T, NewErrorType>;
        if (self.HasError())
        {
            return ResultT{ Unexpected(std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error())) };
        }
        return ResultT{ std::forward<Self>(self).Value() };
    }

public:
    /** Expected가 값을 가지고 있는지 확인합니다. (bool 변환용) */
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return HasValue(); }

    template <typename Self>
    [[nodiscard]] constexpr auto&& operator*(this Self&& self) { return std::forward<Self>(self).Value(); }

    [[nodiscard]] constexpr T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] constexpr const T* operator->() const { return std::addressof(Value()); }

private:
    std::variant<std::monostate, T, E> storage;
};

/** void 특수화 */
template <typename E>
class [[nodiscard]] Expected<void, E>
{
public:
    using ValueType = void;
    using ErrorType = E;
    using UnexpectedType = Unexpected<E>;

public:
    /** 성공 상태의 Expected<void, E>를 생성합니다. */
    constexpr Expected()
        : storage(std::in_place_index<0>, std::monostate{})
    {
    }

    /** 에러 상태의 Expected<void, E>를 생성합니다. */
    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(const Unexpected<Err>& error)
        : storage(std::in_place_index<1>, error.Error())
    {
    }

    template <typename Err = E>
        requires std::constructible_from<E, Err>
    constexpr Expected(Unexpected<Err>&& error)
        : storage(std::in_place_index<1>, std::move(error.Error()))
    {
    }

    constexpr Expected(const Expected&) = default;
    constexpr Expected(Expected&&) noexcept = default;
    constexpr Expected& operator=(const Expected&) = default;
    constexpr Expected& operator=(Expected&&) noexcept = default;

public:
    /** 내부 저장소를 성공 상태로 설정합니다. */
    template <typename... Args>
    constexpr void Emplace([[maybe_unused]] Args&&... args)
    {
        storage.template emplace<0>();
    }

public:
    [[nodiscard]] constexpr bool HasValue() const noexcept { return storage.index() == 0; }
    [[nodiscard]] constexpr bool HasError() const noexcept { return storage.index() == 1; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return HasValue(); }

    /** 성공 상태일 때 호출합니다. (값을 반환하지 않음) */
    constexpr void Value() const
    {
        SE_ASSERT(HasValue(), "Attempted to access value of an Expected<void, E> that contains an error.");
    }

    /** 가지고 있는 에러를 반환합니다. 에러가 없으면 assert로 실패합니다. */
    template <typename Self>
    [[nodiscard]] constexpr auto&& Error(this Self&& self)
    {
        SE_ASSERT(self.HasError(), "Attempted to access error of a successful Expected<void, E>.");
        return std::forward_like<Self>(std::get<1>(self.storage));
    }

    /** 성공 상태일 때, fn() -> Expected<U, E> 함수를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn>, Expected>
    constexpr auto AndThen(Fn&& func) const
    {
        using ResultT = std::invoke_result_t<Fn>;
        if (HasValue())
        {
            return std::invoke(std::forward<Fn>(func));
        }
        return ResultT{ Unexpected(Error()) };
    }

    /** 성공 상태일 때, fn() -> U 함수를 호출하여 새로운 Expected<U, E>를 반환합니다. */
    template <typename Fn>
        requires std::invocable<Fn>
    constexpr auto Map(Fn&& func) const
    {
        using ResultU = std::invoke_result_t<Fn>;
        using ResultT = Expected<ResultU, E>;
        if (HasValue())
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

    /** 에러가 존재할 때, fn(E) -> Expected<void, F> 함수를 호출하여 새로운 Expected<void, F>를 반환합니다. */
    template <typename Self, typename Fn>
        requires se::traits::IsSpecializationOf<std::invoke_result_t<Fn, decltype(std::declval<Self>().Error())>, Expected>
    constexpr auto OrElse(this Self&& self, Fn&& func)
    {
        using ResultT = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        if (self.HasError())
        {
            return std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error());
        }
        return ResultT{};
    }

    /** 에러가 존재할 때, fn(E) -> F 함수를 호출하여 새로운 Expected<void, F>를 반환합니다. */
    template <typename Self, typename Fn>
    constexpr auto MapError(this Self&& self, Fn&& func)
    {
        using NewErrorType = std::invoke_result_t<Fn, decltype(std::forward<Self>(self).Error())>;
        using ResultT = Expected<void, NewErrorType>;
        if (self.HasError())
        {
            return ResultT{ Unexpected(std::invoke(std::forward<Fn>(func), std::forward<Self>(self).Error())) };
        }
        return ResultT{};
    }

private:
    std::variant<std::monostate, E> storage;
};

/**
 * 함수의 실패를 나타내는 래퍼 클래스
 * @detail Expected<T, E>에 에러 값을 전달하기 위해 사용됩니다.
 */
template <typename E>
class Unexpected
{
public:
    using ErrorType = E;

public:
    template <typename Err = E>
        requires(!std::same_as<std::remove_cvref_t<Err>, Unexpected>)
        && std::constructible_from<E, Err>
    constexpr explicit Unexpected(Err&& error)
        : error_value(std::forward<Err>(error))
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

}  // namespace se
