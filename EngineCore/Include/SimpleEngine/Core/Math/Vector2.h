#pragma once
#include <cassert>

#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se::math
{
template <traits::FloatingType T>
struct Vector2Impl
{
    T x, y;

public:
    constexpr Vector2Impl() = default;
    constexpr Vector2Impl(T in_x, T in_y);
    explicit constexpr Vector2Impl(T scalar);

public:
    /** Vector(0, 0) */
    [[nodiscard]] static constexpr Vector2Impl Zero();

    /** Vector(1, 1) */
    [[nodiscard]] static constexpr Vector2Impl One();

public:
    /** Dot Product */
    [[nodiscard]] constexpr T operator|(const Vector2Impl& other) const;
    [[nodiscard]] constexpr T Dot(const Vector2Impl& other) const;
    [[nodiscard]] static constexpr T DotProduct(const Vector2Impl& a, const Vector2Impl& b);

    [[nodiscard]] constexpr T Dist(const Vector2Impl& other);
    [[nodiscard]] static constexpr T Distance(const Vector2Impl& a, const Vector2Impl& b);
    [[nodiscard]] static constexpr T DistSquared(const Vector2Impl& a, const Vector2Impl& b);

    [[nodiscard]] constexpr Vector2Impl operator+(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator+(T scalar) const;
    friend constexpr Vector2Impl operator+(T scalar, const Vector2Impl& self);
    constexpr Vector2Impl& operator+=(const Vector2Impl& other);
    constexpr Vector2Impl& operator+=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator-(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator-(T scalar) const;
    constexpr Vector2Impl& operator-=(const Vector2Impl& other);
    constexpr Vector2Impl& operator-=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator*(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator*(T scalar) const;
    friend constexpr Vector2Impl operator*(T scalar, const Vector2Impl& self);
    constexpr Vector2Impl& operator*=(const Vector2Impl& other);
    constexpr Vector2Impl& operator*=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator/(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator/(T scalar) const;
    constexpr Vector2Impl& operator/=(const Vector2Impl& other);
    constexpr Vector2Impl& operator/=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator-() const;

    [[nodiscard]] constexpr bool operator==(const Vector2Impl& other) const;
    [[nodiscard]] constexpr bool operator!=(const Vector2Impl& other) const;

    [[nodiscard]] constexpr T operator[](usize index);
    [[nodiscard]] constexpr T operator[](usize index) const;

public:
    [[nodiscard]] constexpr T Length() const;
    [[nodiscard]] constexpr T SquaredLength() const;

    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr Vector2Impl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] bool IsNearlyZero(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] bool IsNearlyEqual(const Vector2Impl& other, T tolerance = KINDA_SMALL_NUMBER) const;
};


template <traits::FloatingType T>
constexpr Vector2Impl<T>::Vector2Impl(T in_x, T in_y)
    : x(in_x), y(in_y)
{
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>::Vector2Impl(T scalar)
    : x(scalar), y(scalar)
{
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::Zero()
{
    return Vector2Impl{};
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::One()
{
    return Vector2Impl{ 1 };
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::operator|(const Vector2Impl& other) const
{
    return x * other.x + y * other.y;
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::Dot(const Vector2Impl& other) const
{
    return *this | other;
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::DotProduct(const Vector2Impl& a, const Vector2Impl& b)
{
    return a | b;
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::Dist(const Vector2Impl& other)
{
    return Distance(*this, other);
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::Distance(const Vector2Impl& a, const Vector2Impl& b)
{
    return Sqrt(DistSquared(a, b));
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::DistSquared(const Vector2Impl& a, const Vector2Impl& b)
{
    return Square(b.x - a.x)
        + Square(b.y - a.y);
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator+(const Vector2Impl& other) const
{
    return Vector2Impl{ x + other.x, y + other.y };
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator+(T scalar) const
{
    return Vector2Impl{ x + scalar, y + scalar };
}

template <traits::FloatingType T>
[[nodiscard]] constexpr Vector2Impl<T> operator+(T scalar, const Vector2Impl<T>& self)
{
    return self + scalar;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator+=(const Vector2Impl& other)
{
    x += other.x;
    y += other.y;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator+=(T scalar)
{
    x += scalar;
    y += scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator-(const Vector2Impl& other) const
{
    return Vector2Impl{ x - other.x, y - other.y };
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator-(T scalar) const
{
    return Vector2Impl{ x - scalar, y - scalar };
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator-=(const Vector2Impl& other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator-=(T scalar)
{
    x -= scalar;
    y -= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator*(const Vector2Impl& other) const
{
    return Vector2Impl{ x * other.x, y * other.y };
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator*(T scalar) const
{
    return Vector2Impl{ x * scalar, y * scalar };
}

template <traits::FloatingType T>
[[nodiscard]] constexpr Vector2Impl<T> operator*(T scalar, const Vector2Impl<T>& self)
{
    return self * scalar;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator*=(const Vector2Impl& other)
{
    x *= other.x;
    y *= other.y;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator/(const Vector2Impl& other) const
{
    return Vector2Impl{ x / other.x, y / other.y };
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator/(T scalar) const
{
    return Vector2Impl{ x / scalar, y / scalar };
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator/=(const Vector2Impl& other)
{
    x /= other.x;
    y /= other.y;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator/=(T scalar)
{
    x /= scalar;
    y /= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator-() const
{
    return Vector2Impl{ -x, -y };
}

template <traits::FloatingType T>
constexpr bool Vector2Impl<T>::operator==(const Vector2Impl& other) const
{
    return x == other.x && y == other.y;
}

template <traits::FloatingType T>
constexpr bool Vector2Impl<T>::operator!=(const Vector2Impl& other) const
{
    return !(*this == other);
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::operator[](usize index)
{
    assert(index < 2);
    return (&x)[index];
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::operator[](usize index) const
{
    assert(index < 2);
    return (&x)[index];
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::Length() const
{
    return Sqrt(SquaredLength());
}

template <traits::FloatingType T>
constexpr T Vector2Impl<T>::SquaredLength() const
{
    return Square(x)
        + Square(y);
}

template <traits::FloatingType T>
constexpr void Vector2Impl<T>::Normalize(T tolerance)
{
    T len = Length();
    if (len > tolerance)
    {
        x /= len;
        y /= len;
    }
    else
    {
        x = y = 0;
    }
}

template <traits::FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::GetNormalized(T tolerance) const
{
    Vector2Impl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <traits::FloatingType T>
constexpr bool Vector2Impl<T>::IsNormalized(T tolerance) const
{
    return Abs(1.f - SquaredLength()) < tolerance;
}

template <traits::FloatingType T>
bool Vector2Impl<T>::IsNearlyZero(T tolerance) const
{
    return Abs(x) <= tolerance
        && Abs(y) <= tolerance;
}

template <traits::FloatingType T>
bool Vector2Impl<T>::IsNearlyEqual(const Vector2Impl& other, T tolerance) const
{
    return Abs(x - other.x) <= tolerance
        && Abs(y - other.y) <= tolerance;
}
}
