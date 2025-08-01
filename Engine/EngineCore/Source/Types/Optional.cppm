export module SimpleEngine.Types:Optional;
import :PlatformTypes;

import SimpleEngine.TypeTraits;
import std;

using namespace se::type_traits;


export template <typename T>
class Optional
{
public:
    Optional() noexcept = default;
    ~Optional() { reset(); }

    template <typename U = T>
        requires std::constructible_from<U, T>
    Optional(std::optional<U>&& other_optional)
    {
        if (other_optional.has_value())
        {
            construct(std::move(other_optional.value()));
        }
    }

    template <typename... Args>
    Optional(std::in_place_t, Args&&... args)
    {
        construct(std::forward<Args>(args)...);
    }

    Optional(std::nullopt_t) noexcept
        : storage{}
    {
    }

    Optional(const T& in_value)
    {
        construct(in_value);
    }

    Optional(T&& in_value)
    {
        construct(std::move(in_value));
    }

    Optional(const Optional& other)
    {
        if (other.has_value())
        {
            construct(other.get_stored_value());
        }
    }

    Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (other.has_value())
        {
            construct(std::move(other.get_stored_value()));
            other.reset();
        }
    }

    Optional& operator=(const T& in_value)
    {
        if (has_value())
        {
            get_stored_value() = in_value;
        }
        else
        {
            construct(in_value);
        }
        return *this;
    }

    Optional& operator=(T&& in_value) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
    {
        if (has_value())
        {
            get_stored_value() = std::move(in_value);
        }
        else
        {
            construct(std::move(in_value));
        }
        return *this;
    }

    Optional& operator=(const Optional& other)
    {
        if (this != &other)
        {
            reset();
            if (other.has_value())
            {
                construct(other.get_stored_value());
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            if (other.has_value())
            {
                construct(std::move(other.get_stored_value()));
                other.reset();
            }
        }
        return *this;
    }

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T& value() &
    {
        if (!has_value())
        {
            throw std::bad_optional_access{};
        }
        return get_stored_value();
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] const T& value() const &
    {
        if (!has_value())
        {
            throw std::bad_optional_access{};
        }
        return get_stored_value();
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T&& value() &&
    {
        if (!has_value())
        {
            throw std::bad_optional_access{};
        }
        return std::move(get_stored_value());
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] const T&& value() const &&
    {
        if (!has_value())
        {
            throw std::bad_optional_access{};
        }
        return std::move(get_stored_value());
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename U = std::remove_cv_t<T>>
        requires std::convertible_to<U, std::remove_cv_t<T>>
    [[nodiscard]] std::remove_cv_t<T> value_or(U&& default_value) const &
    {
        if (has_value())
        {
            return get_stored_value();
        }
        return static_cast<std::remove_cv_t<T>>(std::forward<U>(default_value));
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    template <typename U = std::remove_cv_t<T>>
        requires std::convertible_to<U, std::remove_cv_t<T>>
    [[nodiscard]] std::remove_cv_t<T> value_or(U&& default_value) &&
    {
        if (has_value())
        {
            return get_stored_value();
        }
        return static_cast<std::remove_cv_t<T>>(std::forward<U>(default_value));
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && IsSpecializationOf<std::invoke_result_t<Fn, const T&>, Optional>
    auto and_then(Fn&& func) const &
    {
        using ResultT = std::invoke_result_t<Fn, const T&>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), get_stored_value());
        }
        return std::remove_cvref_t<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, T&&>
        && IsSpecializationOf<std::invoke_result_t<Fn, T>, Optional>
    auto and_then(Fn&& func) &&
    {
        using ResultT = std::invoke_result_t<Fn, T>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), std::move(get_stored_value()));
        }
        return std::remove_cvref_t<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&&>
        && IsSpecializationOf<std::invoke_result_t<Fn, const T>, Optional>
    auto and_then(Fn&& func) const &&
    {
        using ResultT = std::invoke_result_t<Fn, const T>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), static_cast<const T&&>(get_stored_value()));
        }
        return std::remove_cvref_t<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, T&>
        && !TIsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T&>>, std::nullopt_t, std::in_place_t>
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
        && !std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
    auto transform(Fn&& func) &
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T&>>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), get_stored_value());
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && !TIsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>, std::nullopt_t, std::in_place_t>
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
        && !std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
    auto transform(Fn&& func) const &
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, const T&>>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), get_stored_value());
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, T&&>
        && !TIsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T>>, std::nullopt_t, std::in_place_t>
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, T>>>
        && !std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, T>>>
    auto transform(Fn&& func) &&
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T>>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), std::move(get_stored_value()));
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<T, Fn, const T&&>
        && !TIsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T>>, std::nullopt_t, std::in_place_t>
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, const T>>>
        && !std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, const T>>>
    auto transform(Fn&& func) const &&
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, const T>>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), std::move(get_stored_value()));
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn>
        && std::copy_constructible<T>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional or_else(Fn&& func) const &
    {
        if (has_value())
        {
            return *this;
        }
        return std::forward<Fn>(func)();
    }

    template <typename Fn>
        requires std::invocable<Fn>
        && std::move_constructible<T>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional or_else(Fn&& func) &&
    {
        if (has_value())
        {
            return std::move(*this);
        }
        return std::forward<Fn>(func)();
    }

    template <typename... Args>
    T& emplace(Args&&... args)
    {
        reset();
        construct(std::forward<Args>(args)...);
        return get_stored_value();
    }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] bool has_value() const noexcept { return is_value_set; }

    void reset()
    {
        if (has_value())
        {
            destroy();
            is_value_set = false;
        }
    }

public:
    [[nodiscard]] T& operator*() { return value(); }
    [[nodiscard]] const T& operator*() const { return value(); }
    [[nodiscard]] T* operator->() { return std::addressof(value()); }
    [[nodiscard]] const T* operator->() const { return std::addressof(value()); }

    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] bool operator==(std::nullopt_t) const noexcept { return !has_value(); }

    [[nodiscard]] bool operator==(const Optional& other) const
    {
        if (has_value() && other.has_value())
        {
            return get_stored_value() == other.get_stored_value();
        }
        return has_value() == other.has_value();
    }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] bool operator==(const U& value) const
    {
        if (has_value())
        {
            return get_stored_value() == value;
        }
        return false;
    }

    [[nodiscard]] friend bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] friend bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    [[nodiscard]] T& get_stored_value() &
    {
        return *std::launder(reinterpret_cast<T*>(&storage));
    }

    [[nodiscard]] const T& get_stored_value() const &
    {
        return *std::launder(reinterpret_cast<const T*>(&storage));
    }

    [[nodiscard]] T&& get_stored_value() &&
    {
        return std::move(*std::launder(reinterpret_cast<T*>(&storage)));
    }

    [[nodiscard]] const T&& get_stored_value() const &&
    {
        return std::move(*std::launder(reinterpret_cast<const T*>(&storage)));
    }

    template <typename... Args>
    void construct(Args&&... args)
    {
        new(storage) T(std::forward<Args>(args)...);
        is_value_set = true;
    }

    void destroy()
    {
        std::destroy_at(std::addressof(get_stored_value()));
    }

private:
    alignas(T) uint8 storage[sizeof(T)];
    bool is_value_set = false;
};

template <typename T>
class Optional<T&>
{
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

    Optional(const Optional&) = default;
    Optional& operator=(const Optional&) noexcept = default;
    Optional(Optional&&) noexcept = default;
    Optional& operator=(Optional&&) noexcept = default;

    // r-value 타입 T는 사용불가
    Optional(T&&) = delete;
    Optional& operator=(T&&) = delete;

public:
    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] T& value()
    {
        if (!has_value())
        {
            throw std::bad_optional_access{};
        }
        return get_stored_value();
    }

    /** Optional이 가지고 있는 값을 반환합니다. */
    [[nodiscard]] const T& value() const
    {
        if (!has_value())
        {
            throw std::bad_optional_access{};
        }
        return get_stored_value();
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] T& value_or(T& default_value)
    {
        if (has_value())
        {
            return get_stored_value();
        }
        return default_value;
    }

    /** Optional이 가지고 있는 값을 반환하거나, 값이 없으면 default_value를 반환합니다. */
    [[nodiscard]] const T& value_or(T& default_value) const
    {
        if (has_value())
        {
            return get_stored_value();
        }
        return default_value;
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && IsSpecializationOf<std::invoke_result_t<Fn, const T&>, Optional>
    auto and_then(Fn&& func) const
    {
        using ResultT = std::invoke_result_t<Fn, const T&>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), get_stored_value());
        }
        return std::remove_cvref_t<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, T&>
        && !TIsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, T&>>, std::nullopt_t, std::in_place_t>
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
        && !std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, T&>>>
    auto transform(Fn&& func)
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, T&>>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), get_stored_value());
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
        && !TIsAnyOf<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>, std::nullopt_t, std::in_place_t>
        && std::is_object_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
        && !std::is_array_v<std::remove_cv_t<std::invoke_result_t<Fn, const T&>>>
    auto transform(Fn&& func) const
    {
        using ResultT = std::remove_cv_t<std::invoke_result_t<Fn, const T&>>;

        if (has_value())
        {
            return std::invoke(std::forward<Fn>(func), get_stored_value());
        }
        return Optional<ResultT>{};
    }

    template <typename Fn>
        requires std::invocable<Fn>
        && std::copy_constructible<T>
        && std::same_as<std::remove_cvref_t<std::invoke_result_t<Fn>>, Optional>
    Optional or_else(Fn&& func) const
    {
        if (has_value())
        {
            return *this;
        }
        return std::forward<Fn>(func)();
    }

    /** TODO: docs */
    void reset() noexcept { ref_ptr = nullptr; }

    /** Optional에 값이 있는지 확인합니다. */
    [[nodiscard]] bool has_value() const noexcept { return ref_ptr != nullptr; }

public:
    [[nodiscard]] T& operator*() { return value(); }
    [[nodiscard]] const T& operator*() const { return value(); }
    [[nodiscard]] T* operator->() const { return ref_ptr; }

    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] bool operator==(std::nullopt_t) const noexcept { return !has_value(); }

    [[nodiscard]] bool operator==(const Optional& other) const
    {
        if (has_value() && other.has_value())
        {
            return get_stored_value() == other.get_stored_value();
        }
        return has_value() == other.has_value();
    }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] bool operator==(const U& value) const
    {
        if (has_value())
        {
            return get_stored_value() == value;
        }
        return false;
    }

    [[nodiscard]] friend bool operator==(std::nullopt_t, const Optional& other) noexcept { return !other; }

    template <typename U = T>
        requires std::convertible_to<U, T>
    [[nodiscard]] friend bool operator==(const U& value, const Optional& other) { return other == value; }

private:
    T& get_stored_value() { return *ref_ptr; }
    const T& get_stored_value() const { return *ref_ptr; }

private:
    T* ref_ptr = nullptr;
};
