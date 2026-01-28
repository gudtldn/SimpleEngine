#pragma once

#include "SimpleEngine/Core/Math/MathFwd.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::math
{
template <traits::FloatingType T>
struct alignas(16) Vector4Impl
{
    T x, y, z, w;

public:
    constexpr Vector4Impl() = default;
    constexpr Vector4Impl(T in_x, T in_y, T in_z, T in_w);
    explicit constexpr Vector4Impl(const Vector3Impl<T>& vector3, T in_w);
    explicit constexpr Vector4Impl(T scalar);

public:
    /** Vector(0, 0, 0, 0) */
    [[nodiscard]] static constexpr Vector4Impl Zero();

    /** Vector(1, 1, 1, 1) */
    [[nodiscard]] static constexpr Vector4Impl One();

public:
    [[nodiscard]] constexpr Vector4Impl operator+(const Vector4Impl& other) const;
    [[nodiscard]] constexpr Vector4Impl operator+(T scalar) const;
    friend constexpr Vector4Impl operator+(T scalar, const Vector4Impl& self);
    constexpr Vector4Impl& operator+=(const Vector4Impl& other);
    constexpr Vector4Impl& operator+=(T scalar);

    [[nodiscard]] constexpr Vector4Impl operator-(const Vector4Impl& other) const;
    [[nodiscard]] constexpr Vector4Impl operator-(T scalar) const;
    constexpr Vector4Impl& operator-=(const Vector4Impl& other);
    constexpr Vector4Impl& operator-=(T scalar);

    [[nodiscard]] constexpr Vector4Impl operator*(const Vector4Impl& other) const;
    [[nodiscard]] constexpr Vector4Impl operator*(T scalar) const;
    friend constexpr Vector4Impl operator*(T scalar, const Vector4Impl& self);
    constexpr Vector4Impl& operator*=(const Vector4Impl& other);
    constexpr Vector4Impl& operator*=(T scalar);

    [[nodiscard]] constexpr Vector4Impl operator/(const Vector4Impl& other) const;
    [[nodiscard]] constexpr Vector4Impl operator/(T scalar) const;
    constexpr Vector4Impl& operator/=(const Vector4Impl& other);
    constexpr Vector4Impl& operator/=(T scalar);

    [[nodiscard]] constexpr Vector4Impl operator-() const;

    [[nodiscard]] constexpr bool operator==(const Vector4Impl& other) const;
    [[nodiscard]] constexpr bool operator!=(const Vector4Impl& other) const;

    [[nodiscard]] constexpr T& operator[](usize index);
    [[nodiscard]] constexpr T operator[](usize index) const;

public:
    [[nodiscard]] constexpr T Length3() const;
    [[nodiscard]] constexpr T Length() const;
    [[nodiscard]] constexpr T SquaredLength3() const;
    [[nodiscard]] constexpr T SquaredLength() const;

    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr Vector4Impl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] bool IsNearlyZero3(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] bool IsNearlyEqual3(const Vector4Impl& other, T tolerance = KINDA_SMALL_NUMBER) const;
};


template <traits::FloatingType T>
constexpr Vector4Impl<T>::Vector4Impl(T in_x, T in_y, T in_z, T in_w)
    : x(in_x), y(in_y), z(in_z), w(in_w)
{
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>::Vector4Impl(const Vector3Impl<T>& vector3, T in_w)
    : x(vector3.x), y(vector3.y), z(vector3.z), w(in_w)
{
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>::Vector4Impl(T scalar)
    : x(scalar), y(scalar), z(scalar), w(scalar)
{
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::Zero()
{
    return Vector4Impl{};
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::One()
{
    return Vector4Impl{ 1 };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator+(const Vector4Impl& other) const
{
    return Vector4Impl{ x + other.x, y + other.y, z + other.z, w + other.w };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator+(T scalar) const
{
    return Vector4Impl{ x + scalar, y + scalar, z + scalar, w + scalar };
}

template <traits::FloatingType T>
[[nodiscard]] constexpr Vector4Impl<T> operator+(T scalar, const Vector4Impl<T>& self)
{
    return self + scalar;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator+=(const Vector4Impl& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator+=(T scalar)
{
    x += scalar;
    y += scalar;
    z += scalar;
    w += scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator-(const Vector4Impl& other) const
{
    return Vector4Impl{ x - other.x, y - other.y, z - other.z, w - other.w };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator-(T scalar) const
{
    return Vector4Impl{ x - scalar, y - scalar, z - scalar, w - scalar };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator-=(const Vector4Impl& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator-=(T scalar)
{
    x -= scalar;
    y -= scalar;
    z -= scalar;
    w -= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator*(const Vector4Impl& other) const
{
    return Vector4Impl{ x * other.x, y * other.y, z * other.z, w * other.w };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator*(T scalar) const
{
    return Vector4Impl{ x * scalar, y * scalar, z * scalar, w * scalar };
}

template <traits::FloatingType T>
[[nodiscard]] constexpr Vector4Impl<T> operator*(T scalar, const Vector4Impl<T>& self)
{
    return self * scalar;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator*=(const Vector4Impl& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator/(const Vector4Impl& other) const
{
    return Vector4Impl{ x / other.x, y / other.y, z / other.z, w / other.w };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator/(T scalar) const
{
    return Vector4Impl{ x / scalar, y / scalar, z / scalar, w / scalar };
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator/=(const Vector4Impl& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T>& Vector4Impl<T>::operator/=(T scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    w /= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::operator-() const
{
    return Vector4Impl{ -x, -y, -z, -w };
}

template <traits::FloatingType T>
constexpr bool Vector4Impl<T>::operator==(const Vector4Impl& other) const
{
    return x == other.x && y == other.y && z == other.z && w == other.w;
}

template <traits::FloatingType T>
constexpr bool Vector4Impl<T>::operator!=(const Vector4Impl& other) const
{
    return !(*this == other);
}

template <traits::FloatingType T>
constexpr T& Vector4Impl<T>::operator[](usize index)
{
    SE_ASSERT(index < 4);
    return (&x)[index];
}

template <traits::FloatingType T>
constexpr T Vector4Impl<T>::operator[](usize index) const
{
    SE_ASSERT(index < 4);
    return (&x)[index];
}

template <traits::FloatingType T>
constexpr T Vector4Impl<T>::Length3() const
{
    return Sqrt(SquaredLength3());
}

template <traits::FloatingType T>
constexpr T Vector4Impl<T>::Length() const
{
    return Sqrt(SquaredLength());
}

template <traits::FloatingType T>
constexpr T Vector4Impl<T>::SquaredLength3() const
{
    return Square(x)
        + Square(y)
        + Square(z);
}

template <traits::FloatingType T>
constexpr T Vector4Impl<T>::SquaredLength() const
{
    return SquaredLength3() + Square(w);
}

template <traits::FloatingType T>
constexpr void Vector4Impl<T>::Normalize(T tolerance)
{
    T len = Length3();
    if (len > tolerance)
    {
        x /= len;
        y /= len;
        z /= len;
    }
    else
    {
        x = y = z = 0;
    }
    w = 0;
}

template <traits::FloatingType T>
constexpr Vector4Impl<T> Vector4Impl<T>::GetNormalized(T tolerance) const
{
    Vector4Impl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <traits::FloatingType T>
constexpr bool Vector4Impl<T>::IsNormalized(T tolerance) const
{
    return Abs(1.f - SquaredLength3()) < tolerance;
}

template <traits::FloatingType T>
bool Vector4Impl<T>::IsNearlyZero3(T tolerance) const
{
    return Abs(x) <= tolerance
        && Abs(y) <= tolerance
        && Abs(z) <= tolerance;
}

template <traits::FloatingType T>
bool Vector4Impl<T>::IsNearlyEqual3(const Vector4Impl& other, T tolerance) const
{
    return Abs(x - other.x) <= tolerance
        && Abs(y - other.y) <= tolerance
        && Abs(z - other.z) <= tolerance;
}
}  // namespace se::math
