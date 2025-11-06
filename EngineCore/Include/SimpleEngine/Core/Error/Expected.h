#pragma once
#include <cassert>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "SimpleEngine/Traits/TypeTraits.h"

// Forward declaration
template <typename E>
class Unexpected;


template <typename T, typename E>
class Expected
{
public:
    using ValueType = T;
    using ErrorType = E;
    using UnexpectedType = Unexpected<E>;

public:
    /** Uninitialized 상태로 Expected를 생성합니다. */
    Expected()
        : storage(std::in_place_index<0>, std::monostate{})
    {
    }

    /** 값이 있는 Expected를 생성합니다. */
    template <typename U = T>
        requires(!std::same_as<std::remove_cvref_t<U>, Expected>)
        && std::constructible_from<T, U>
    Expected(U&& value)
        : storage(std::in_place_index<1>, std::forward<U>(value))
    {
    }

    /** 에러가 있는 Expected를 생성합니다. */
    template <typename Err = E>
        requires std::constructible_from<E, Err>
    Expected(const Unexpected<Err>& error)
        : storage(std::in_place_index<2>, error.Error())
    {
    }

    template <typename Err = E>
        requires std::constructible_from<E, Err>
    Expected(Unexpected<Err>&& error)
        : storage(std::in_place_index<2>, std::move(error.Error()))
    {
    }

    /** 값을 내부 생성하여 Expected를 생성합니다. */
    template <typename... Args>
    Expected(std::in_place_t, Args&&... args)
        : storage(std::in_place_index<1>, std::forward<Args>(args)...)
    {
    }

    Expected(const Expected&) = default;
    Expected(Expected&&) noexcept = default;
    Expected& operator=(const Expected&) = default;
    Expected& operator=(Expected&&) noexcept = default;

public:
    /** 내부 저장소에 새로운 값을 직접 생성(in-place)합니다. */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    T& Emplace(Args&&... args);

public:
    /** Expected가 값을 가지고 있는지 확인합니다. */
    [[nodiscard]] bool HasValue() const noexcept { return storage.index() == 1; }

    /** Expected가 에러를 가지고 있는지 확인합니다. */
    [[nodiscard]] bool HasError() const noexcept { return storage.index() == 2; }

    /** 가지고 있는 값을 반환합니다. 값이 없으면 assert로 실패합니다. */
    [[nodiscard]] T& Value() &;
    [[nodiscard]] const T& Value() const &;
    [[nodiscard]] T&& Value() &&;
    [[nodiscard]] const T&& Value() const &&;

    /** 가지고 있는 에러를 반환합니다. 에러가 없으면 assert로 실패합니다. */
    [[nodiscard]] E& Error() &;
    [[nodiscard]] const E& Error() const &;
    [[nodiscard]] E&& Error() &&;
    [[nodiscard]] const E&& Error() const &&;

    /** 가지고 있는 값을 반환하거나, 에러가 있으면 default_value를 반환합니다. */
    template <typename U>
        requires std::copy_constructible<T> && std::convertible_to<U&&, T>
    [[nodiscard]] T ValueOr(U&& default_value) const &;
    template <typename U>
        requires std::move_constructible<T> && std::convertible_to<U&&, T>
    [[nodiscard]] T ValueOr(U&& default_value) &&;

    template <typename Fn>
        requires std::invocable<Fn, T&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, T&>, Expected>
    auto AndThen(Fn&& func) &;
    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T&>, Expected>
    auto AndThen(Fn&& func) const &;
    template <typename Fn>
        requires std::invocable<Fn, T&&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, T&&>, Expected>
    auto AndThen(Fn&& func) &&;
    template <typename Fn>
        requires std::invocable<Fn, const T&&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T&&>, Expected>
    auto AndThen(Fn&& func) const &&;

    /**
     * 값이 존재할 때, fn(T) -> U 함수를 호출하여 새로운 Expected<U, E>를 반환합니다.
     * 에러가 있으면, 에러를 그대로 전달합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn, T&>
    auto Map(Fn&& func) &;
    template <typename Fn>
        requires std::invocable<Fn, const T&>
    auto Map(Fn&& func) const &;
    template <typename Fn>
        requires std::invocable<Fn, T&&>
    auto Map(Fn&& func) &&;
    template <typename Fn>
        requires std::invocable<Fn, const T&&>
    auto Map(Fn&& func) const &&;

    /**
     * 에러가 존재할 때, fn(E) -> Expected<T, F> 함수를 호출하여 새로운 Expected<T, F>를 반환합니다.
     * 값이 있으면, 값을 그대로 전달합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn, E&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&>, Expected>
    auto OrElse(Fn&& func) &;
    template <typename Fn>
        requires std::invocable<Fn, E&&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&&>, Expected>
    auto OrElse(Fn&& func) &&;

    /**
     * 에러가 존재할 때, fn(E) -> F 함수를 호출하여 새로운 Expected<T, F>를 반환합니다.
     * 값이 있으면, 값을 그대로 전달합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn, E&>
    auto MapError(Fn&& func) &;
    template <typename Fn>
        requires std::invocable<Fn, E&&>
    auto MapError(Fn&& func) &&;

public:
    /** Expected가 값을 가지고 있는지 확인합니다. (bool 변환용) */
    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    [[nodiscard]] T& operator*() & { return Value(); }
    [[nodiscard]] const T& operator*() const & { return Value(); }
    [[nodiscard]] T&& operator*() && { return std::move(Value()); }
    [[nodiscard]] const T&& operator*() const && { return std::move(Value()); }

    [[nodiscard]] T* operator->() { return std::addressof(Value()); }
    [[nodiscard]] const T* operator->() const { return std::addressof(Value()); }

private:
    std::variant<std::monostate, T, E> storage;
};

/** void 특수화 */
template <typename E>
class Expected<void, E>
{
public:
    using ValueType = void;
    using ErrorType = E;
    using UnexpectedType = Unexpected<E>;

public:
    /** 성공 상태의 Expected<void, E>를 생성합니다. */
    Expected()
        : storage(std::in_place_index<0>, std::monostate{})
    {
    }

    /** 에러 상태의 Expected<void, E>를 생성합니다. */
    template <typename Err = E>
        requires std::constructible_from<E, Err>
    Expected(const Unexpected<Err>& error)
        : storage(std::in_place_index<1>, error.Error())
    {
    }

    template <typename Err = E>
        requires std::constructible_from<E, Err>
    Expected(Unexpected<Err>&& error)
        : storage(std::in_place_index<1>, std::move(error.Error()))
    {
    }

    Expected(const Expected&) = default;
    Expected(Expected&&) noexcept = default;
    Expected& operator=(const Expected&) = default;
    Expected& operator=(Expected&&) noexcept = default;

public:
    /** 내부 저장소에 새로운 값을 직접 생성(in-place)합니다. */
    template <typename... Args>
    void Emplace(Args&&... args);

public:
    [[nodiscard]] bool HasValue() const noexcept { return storage.index() == 0; }
    [[nodiscard]] bool HasError() const noexcept { return storage.index() == 1; }
    [[nodiscard]] explicit operator bool() const noexcept { return HasValue(); }

    /** 성공 상태일 때 호출합니다. (값을 반환하지 않음) */
    void Value() const { assert(HasValue() && "Attempted to access value of an Expected<void, E> that contains an error."); }

    /** 가지고 있는 에러를 반환합니다. 에러가 없으면 assert로 실패합니다. */
    [[nodiscard]] E& Error() &;
    [[nodiscard]] const E& Error() const &;
    [[nodiscard]] E&& Error() &&;
    [[nodiscard]] const E&& Error() const &&;

    /**
     * 성공 상태일 때, fn() -> Expected<U, E> 함수를 호출하여 새로운 Expected<U, E>를 반환합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn>, Expected>
    auto AndThen(Fn&& func) const;

    /**
     * 성공 상태일 때, fn() -> U 함수를 호출하여 새로운 Expected<U, E>를 반환합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn>
    auto Map(Fn&& func) const;

    /**
     * 에러가 존재할 때, fn(E) -> Expected<void, F> 함수를 호출하여 새로운 Expected<void, F>를 반환합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn, E&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&>, Expected>
    auto OrElse(Fn&& func) &;
    template <typename Fn>
        requires std::invocable<Fn, E&&>
        && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&&>, Expected>
    auto OrElse(Fn&& func) &&;

    /**
     * 에러가 존재할 때, fn(E) -> F 함수를 호출하여 새로운 Expected<void, F>를 반환합니다.
     */
    template <typename Fn>
        requires std::invocable<Fn, E&>
    auto MapError(Fn&& func) &;
    template <typename Fn>
        requires std::invocable<Fn, E&&>
    auto MapError(Fn&& func) &&;

private:
    std::variant<std::monostate, E> storage;
};

/**
 * 함수의 실패를 나타내는 래퍼 클래스
 * @details Expected<T, E>에 에러 값을 전달하기 위해 사용됩니다.
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
    explicit Unexpected(Err&& error)
        : error_value(std::forward<Err>(error))
    {
    }

    Unexpected(const Unexpected&) = default;
    Unexpected(Unexpected&&) noexcept = default;
    Unexpected& operator=(const Unexpected&) = default;
    Unexpected& operator=(Unexpected&&) noexcept = default;

public:
    /** 저장된 에러 값을 반환합니다. */
    [[nodiscard]] E& Error() & noexcept { return error_value; }
    [[nodiscard]] const E& Error() const & noexcept { return error_value; }
    [[nodiscard]] E&& Error() && noexcept { return std::move(error_value); }
    [[nodiscard]] const E&& Error() const && noexcept { return std::move(error_value); }

private:
    E error_value;
};

template <typename E>
Unexpected(E) -> Unexpected<E>;


template <typename T, typename E>
template <typename... Args> requires std::constructible_from<T, Args...>
T& Expected<T, E>::Emplace(Args&&... args)
{
    return storage.template emplace<1>(std::forward<Args>(args)...);
}

template <typename T, typename E>
T& Expected<T, E>::Value() &
{
    assert(HasValue() && "Attempted to access value of an Expected that contains an error.");
    return std::get<1>(storage);
}

template <typename T, typename E>
const T& Expected<T, E>::Value() const &
{
    assert(HasValue() && "Attempted to access value of an Expected that contains an error.");
    return std::get<1>(storage);
}

template <typename T, typename E>
T&& Expected<T, E>::Value() &&
{
    assert(HasValue() && "Attempted to access value of an Expected that contains an error.");
    return std::move(std::get<1>(storage));
}

template <typename T, typename E>
const T&& Expected<T, E>::Value() const &&
{
    assert(HasValue() && "Attempted to access value of an Expected that contains an error.");
    return std::move(std::get<1>(storage));
}

template <typename T, typename E>
E& Expected<T, E>::Error() &
{
    assert(HasError() && "Attempted to access error of an Expected that contains a value.");
    return std::get<2>(storage);
}

template <typename T, typename E>
const E& Expected<T, E>::Error() const &
{
    assert(HasError() && "Attempted to access error of an Expected that contains a value.");
    return std::get<2>(storage);
}

template <typename T, typename E>
E&& Expected<T, E>::Error() &&
{
    assert(HasError() && "Attempted to access error of an Expected that contains a value.");
    return std::move(std::get<2>(storage));
}

template <typename T, typename E>
const E&& Expected<T, E>::Error() const &&
{
    assert(HasError() && "Attempted to access error of an Expected that contains a value.");
    return std::move(std::get<2>(storage));
}

template <typename T, typename E>
template <typename U>
    requires std::copy_constructible<T> && std::convertible_to<U&&, T>
T Expected<T, E>::ValueOr(U&& default_value) const &
{
    if (HasValue())
    {
        return Value();
    }
    return static_cast<T>(std::forward<U>(default_value));
}

template <typename T, typename E>
template <typename U>
    requires std::move_constructible<T> && std::convertible_to<U&&, T>
T Expected<T, E>::ValueOr(U&& default_value) &&
{
    if (HasValue())
    {
        return std::move(Value());
    }
    return static_cast<T>(std::forward<U>(default_value));
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, T&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, T&>, Expected>
auto Expected<T, E>::AndThen(Fn&& func) &
{
    if (HasValue())
    {
        return std::invoke(std::forward<Fn>(func), Value());
    }
    // func의 반환 타입에서 에러 타입을 추론하여 Unexpected를 생성
    using RetType = std::invoke_result_t<Fn, T&>;
    return RetType{ Unexpected(Error()) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, const T&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T&>, Expected>
auto Expected<T, E>::AndThen(Fn&& func) const &
{
    if (HasValue())
    {
        return std::invoke(std::forward<Fn>(func), Value());
    }
    using RetType = std::invoke_result_t<Fn, const T&>;
    return RetType{ Unexpected(Error()) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, T&&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, T&&>, Expected>
auto Expected<T, E>::AndThen(Fn&& func) &&
{
    if (HasValue())
    {
        return std::invoke(std::forward<Fn>(func), std::move(Value()));
    }
    using RetType = std::invoke_result_t<Fn, T&&>;
    return RetType{ Unexpected(std::move(Error())) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, const T&&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, const T&&>, Expected>
auto Expected<T, E>::AndThen(Fn&& func) const &&
{
    if (HasValue())
    {
        return std::invoke(std::forward<Fn>(func), std::move(Value()));
    }
    using RetType = std::invoke_result_t<Fn, const T&&>;
    return RetType{ Unexpected(std::move(Error())) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, T&>
auto Expected<T, E>::Map(Fn&& func) &
{
    using RetType = Expected<std::invoke_result_t<Fn, T&>, E>;
    if (HasValue())
    {
        return RetType{ std::invoke(std::forward<Fn>(func), Value()) };
    }
    return RetType{ Unexpected(Error()) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, const T&>
auto Expected<T, E>::Map(Fn&& func) const &
{
    using RetType = Expected<std::invoke_result_t<Fn, const T&>, E>;
    if (HasValue())
    {
        return RetType{ std::invoke(std::forward<Fn>(func), Value()) };
    }
    return RetType{ Unexpected(Error()) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, T&&>
auto Expected<T, E>::Map(Fn&& func) &&
{
    using RetType = Expected<std::invoke_result_t<Fn, T&&>, E>;
    if (HasValue())
    {
        return RetType{ std::invoke(std::forward<Fn>(func), std::move(Value())) };
    }
    return RetType{ Unexpected(std::move(Error())) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, const T&&>
auto Expected<T, E>::Map(Fn&& func) const &&
{
    using RetType = Expected<std::invoke_result_t<Fn, const T&&>, E>;
    if (HasValue())
    {
        return RetType{ std::invoke(std::forward<Fn>(func), std::move(Value())) };
    }
    return RetType{ Unexpected(std::move(Error())) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, E&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&>, Expected>
auto Expected<T, E>::OrElse(Fn&& func) &
{
    if (HasError())
    {
        return std::invoke(std::forward<Fn>(func), Error());
    }
    using RetType = std::invoke_result_t<Fn, E&>;
    return RetType{ Value() };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, E&&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&&>, Expected>
auto Expected<T, E>::OrElse(Fn&& func) &&
{
    if (HasError())
    {
        return std::invoke(std::forward<Fn>(func), std::move(Error()));
    }
    using RetType = std::invoke_result_t<Fn, E&&>;
    return RetType{ std::move(Value()) };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, E&>
auto Expected<T, E>::MapError(Fn&& func) &
{
    using NewErrorType = std::invoke_result_t<Fn, E&>;
    using RetType = Expected<T, NewErrorType>;

    if (HasError())
    {
        return RetType{ Unexpected(std::invoke(std::forward<Fn>(func), Error())) };
    }
    return RetType{ Value() };
}

template <typename T, typename E>
template <typename Fn>
    requires std::invocable<Fn, E&&>
auto Expected<T, E>::MapError(Fn&& func) &&
{
    using NewErrorType = std::invoke_result_t<Fn, E&&>;
    using RetType = Expected<T, NewErrorType>;

    if (HasError())
    {
        return RetType{ Unexpected(std::invoke(std::forward<Fn>(func), std::move(Error()))) };
    }
    return RetType{ std::move(Value()) };
}

template <typename E>
template <typename... Args>
void Expected<void, E>::Emplace([[maybe_unused]] Args&&... args)
{
    storage.template emplace<0>();
}

template <typename E>
E& Expected<void, E>::Error() &
{
    assert(HasError() && "Attempted to access error of a successful Expected<void, E>.");
    return std::get<1>(storage);
}

template <typename E>
const E& Expected<void, E>::Error() const &
{
    assert(HasError() && "Attempted to access error of a successful Expected<void, E>.");
    return std::get<1>(storage);
}

template <typename E>
E&& Expected<void, E>::Error() &&
{
    assert(HasError() && "Attempted to access error of a successful Expected<void, E>.");
    return std::move(std::get<1>(storage));
}

template <typename E>
const E&& Expected<void, E>::Error() const &&
{
    assert(HasError() && "Attempted to access error of a successful Expected<void, E>.");
    return std::move(std::get<1>(storage));
}

template <typename E>
template <typename Fn>
    requires std::invocable<Fn>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn>, Expected>
auto Expected<void, E>::AndThen(Fn&& func) const
{
    if (HasValue())
    {
        return std::invoke(std::forward<Fn>(func));
    }
    using RetType = std::invoke_result_t<Fn>;
    return RetType{ Unexpected(Error()) };
}

template <typename E>
template <typename Fn>
    requires std::invocable<Fn>
auto Expected<void, E>::Map(Fn&& func) const
{
    using RetU = std::invoke_result_t<Fn>;
    using RetType = Expected<RetU, E>;
    if (HasValue())
    {
        // void->U 변환의 경우, U가 void일 수도 있으므로 분기 처리
        if constexpr (std::is_void_v<RetU>)
        {
            std::invoke(std::forward<Fn>(func));
            return RetType();
        }
        else
        {
            return RetType(std::invoke(std::forward<Fn>(func)));
        }
    }
    return RetType{ Unexpected(Error()) };
}

template <typename E>
template <typename Fn>
    requires std::invocable<Fn, E&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&>, Expected>
auto Expected<void, E>::OrElse(Fn&& func) &
{
    if (HasError())
    {
        return std::invoke(std::forward<Fn>(func), Error());
    }
    using RetType = std::invoke_result_t<Fn, E&>;
    return RetType();
}

template <typename E>
template <typename Fn>
    requires std::invocable<Fn, E&&>
    && se::traits::IsSpecializationOf<std::invoke_result_t<Fn, E&&>, Expected>
auto Expected<void, E>::OrElse(Fn&& func) &&
{
    if (HasError())
    {
        return std::invoke(std::forward<Fn>(func), std::move(Error()));
    }
    using RetType = std::invoke_result_t<Fn, E&&>;
    return RetType();
}

template <typename E>
template <typename Fn>
    requires std::invocable<Fn, E&>
auto Expected<void, E>::MapError(Fn&& func) &
{
    using NewErrorType = std::invoke_result_t<Fn, E&>;
    using RetType = Expected<void, NewErrorType>;
    if (HasError())
    {
        return RetType{ Unexpected(std::invoke(std::forward<Fn>(func), Error())) };
    }
    return RetType();
}

template <typename E>
template <typename Fn>
    requires std::invocable<Fn, E&&>
auto Expected<void, E>::MapError(Fn&& func) &&
{
    using NewErrorType = std::invoke_result_t<Fn, E&&>;
    using RetType = Expected<void, NewErrorType>;
    if (HasError())
    {
        return RetType{ Unexpected(std::invoke(std::forward<Fn>(func), std::move(Error()))) };
    }
    return RetType();
}
