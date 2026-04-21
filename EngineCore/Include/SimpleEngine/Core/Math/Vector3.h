#pragma once

#include "SimpleEngine/Core/Math/MathFwd.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::math
{
template <traits::FloatingType T>
struct Vector3Impl
{
    T x, y, z;

public:
    constexpr Vector3Impl() = default;
    constexpr Vector3Impl(T in_x, T in_y, T in_z);
    explicit constexpr Vector3Impl(const Vector4Impl<T>& vector4);
    explicit constexpr Vector3Impl(T scalar);

public:
    /** Vector(0, 0, 0) */
    [[nodiscard]] static constexpr Vector3Impl Zero();

    /** Vector(1, 1, 1) */
    [[nodiscard]] static constexpr Vector3Impl One();

    /** Unit Vector(1, 0, 0) */
    [[nodiscard]] static constexpr Vector3Impl UnitX();

    /** Unit Vector(0, 1, 0) */
    [[nodiscard]] static constexpr Vector3Impl UnitY();

    /** Unit Vector(0, 0, 1) */
    [[nodiscard]] static constexpr Vector3Impl UnitZ();

    /** Unit Vector(0, 0, 1) */
    [[nodiscard]] static constexpr Vector3Impl Up();

    /** Unit Vector(0, 1, 0) */
    [[nodiscard]] static constexpr Vector3Impl Forward();

    /** Unit Vector(1, 0, 0) */
    [[nodiscard]] static constexpr Vector3Impl Right();

public:
    /** Dot Product */
    [[nodiscard]] constexpr T operator|(const Vector3Impl& other) const;
    [[nodiscard]] constexpr T Dot(const Vector3Impl& other) const;
    [[nodiscard]] static constexpr T DotProduct(const Vector3Impl& a, const Vector3Impl& b);

    /** Cross Product */
    [[nodiscard]] constexpr Vector3Impl operator^(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl Cross(const Vector3Impl& other) const;
    [[nodiscard]] static constexpr Vector3Impl CrossProduct(const Vector3Impl& a, const Vector3Impl& b);

    [[nodiscard]] constexpr T Dist(const Vector3Impl& other);
    [[nodiscard]] static constexpr T Distance(const Vector3Impl& a, const Vector3Impl& b);
    [[nodiscard]] static constexpr T Distance2D(const Vector3Impl& a, const Vector3Impl& b);

    [[nodiscard]] static constexpr T DistSquared(const Vector3Impl& a, const Vector3Impl& b);
    [[nodiscard]] static constexpr T DistSquared2D(const Vector3Impl& a, const Vector3Impl& b);

    [[nodiscard]] constexpr Vector3Impl operator+(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator+(T scalar) const;
    [[nodiscard]] friend constexpr Vector3Impl operator+(T scalar, const Vector3Impl& self) { return self + scalar; }
    constexpr Vector3Impl& operator+=(const Vector3Impl& other);
    constexpr Vector3Impl& operator+=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator-(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator-(T scalar) const;
    constexpr Vector3Impl& operator-=(const Vector3Impl& other);
    constexpr Vector3Impl& operator-=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator*(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator*(T scalar) const;
    [[nodiscard]] friend constexpr Vector3Impl operator*(T scalar, const Vector3Impl& self) { return self * scalar; }
    constexpr Vector3Impl& operator*=(const Vector3Impl& other);
    constexpr Vector3Impl& operator*=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator/(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator/(T scalar) const;
    constexpr Vector3Impl& operator/=(const Vector3Impl& other);
    constexpr Vector3Impl& operator/=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator-() const;

    [[nodiscard]] constexpr bool operator==(const Vector3Impl& other) const;
    [[nodiscard]] constexpr bool operator!=(const Vector3Impl& other) const;

    [[nodiscard]] constexpr T operator[](usize index);
    [[nodiscard]] constexpr T operator[](usize index) const;

public:
    [[nodiscard]] constexpr T Length() const;
    [[nodiscard]] constexpr T SquaredLength() const;

    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr Vector3Impl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] bool IsNearlyZero(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] bool IsNearlyEqual(const Vector3Impl& other, T tolerance = KINDA_SMALL_NUMBER) const;

    /**
     * 현재 벡터(법선)에 직교하는 두 개의 축을 계산합니다.
     * @param out_tangent 계산된 첫 번째 직교 축
     * @param out_bitangent 계산된 두 번째 직교 축
     */
    void GetOrthogonalAxes(Vector3Impl& out_tangent, Vector3Impl& out_bitangent) const;
};


template <traits::FloatingType T>
constexpr Vector3Impl<T>::Vector3Impl(T in_x, T in_y, T in_z)
    : x(in_x), y(in_y), z(in_z)
{
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>::Vector3Impl(const Vector4Impl<T>& vector4)
    : x(vector4.x), y(vector4.y), z(vector4.z)
{
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>::Vector3Impl(T scalar)
    : x(scalar), y(scalar), z(scalar)
{
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Zero()
{
    return Vector3Impl{ 0 };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::One()
{
    return Vector3Impl{ 1 };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::UnitX()
{
    return Vector3Impl{ 1, 0, 0 };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::UnitY()
{
    return Vector3Impl{ 0, 1, 0 };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::UnitZ()
{
    return Vector3Impl{ 0, 0, 1 };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Up()
{
    return UnitZ();
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Forward()
{
    return UnitY();
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Right()
{
    return UnitX();
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::operator|(const Vector3Impl& other) const
{
    return (x * other.x) + (y * other.y) + (z * other.z);
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::Dot(const Vector3Impl& other) const
{
    return *this | other;
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::DotProduct(const Vector3Impl& a, const Vector3Impl& b)
{
    return a | b;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator^(const Vector3Impl& other) const
{
    return Vector3Impl{
        (y * other.z) - (z * other.y),
        (z * other.x) - (x * other.z),
        (x * other.y) - (y * other.x)
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Cross(const Vector3Impl& other) const
{
    return *this ^ other;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::CrossProduct(const Vector3Impl& a, const Vector3Impl& b)
{
    return a ^ b;
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::Dist(const Vector3Impl& other)
{
    return Distance(*this, other);
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::Distance(const Vector3Impl& a, const Vector3Impl& b)
{
    return Sqrt(DistSquared(a, b));
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::Distance2D(const Vector3Impl& a, const Vector3Impl& b)
{
    return Sqrt(DistSquared2D(a, b));
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::DistSquared(const Vector3Impl& a, const Vector3Impl& b)
{
    return Square(b.x - a.x)
        + Square(b.y - a.y)
        + Square(b.z - a.z);
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::DistSquared2D(const Vector3Impl& a, const Vector3Impl& b)
{
    return Square(b.x - a.x)
        + Square(b.y - a.y);
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator+(const Vector3Impl& other) const
{
    return Vector3Impl{ x + other.x, y + other.y, z + other.z };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator+(T scalar) const
{
    return Vector3Impl{ x + scalar, y + scalar, z + scalar };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator+=(const Vector3Impl& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator+=(T scalar)
{
    x += scalar;
    y += scalar;
    z += scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator-(const Vector3Impl& other) const
{
    return Vector3Impl{ x - other.x, y - other.y, z - other.z };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator-(T scalar) const
{
    return Vector3Impl{ x - scalar, y - scalar, z - scalar };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator-=(const Vector3Impl& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator-=(T scalar)
{
    x -= scalar;
    y -= scalar;
    z -= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator*(const Vector3Impl& other) const
{
    return Vector3Impl{ x * other.x, y * other.y, z * other.z };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator*(T scalar) const
{
    return Vector3Impl{ x * scalar, y * scalar, z * scalar };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator*=(const Vector3Impl& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator/(const Vector3Impl& other) const
{
    return Vector3Impl{ x / other.x, y / other.y, z / other.z };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator/(T scalar) const
{
    return Vector3Impl{ x / scalar, y / scalar, z / scalar };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator/=(const Vector3Impl& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator/=(T scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator-() const
{
    return Vector3Impl{ -x, -y, -z };
}

template <traits::FloatingType T>
constexpr bool Vector3Impl<T>::operator==(const Vector3Impl& other) const
{
    return x == other.x && y == other.y && z == other.z;
}

template <traits::FloatingType T>
constexpr bool Vector3Impl<T>::operator!=(const Vector3Impl& other) const
{
    return !(*this == other);
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::operator[](usize index)
{
    SE_ASSERT(index < 3);
    return (&x)[index];
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::operator[](usize index) const
{
    SE_ASSERT(index < 3);
    return (&x)[index];
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::Length() const
{
    return Sqrt(SquaredLength());
}

template <traits::FloatingType T>
constexpr T Vector3Impl<T>::SquaredLength() const
{
    return Square(x)
        + Square(y)
        + Square(z);
}

template <traits::FloatingType T>
constexpr void Vector3Impl<T>::Normalize(T tolerance)
{
    T len = Length();
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
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::GetNormalized(T tolerance) const
{
    Vector3Impl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <traits::FloatingType T>
constexpr bool Vector3Impl<T>::IsNormalized(T tolerance) const
{
    return Abs(1.f - SquaredLength()) < tolerance;
}

template <traits::FloatingType T>
bool Vector3Impl<T>::IsNearlyZero(T tolerance) const
{
    return Abs(x) <= tolerance
        && Abs(y) <= tolerance
        && Abs(z) <= tolerance;
}

template <traits::FloatingType T>
bool Vector3Impl<T>::IsNearlyEqual(const Vector3Impl& other, T tolerance) const
{
    return Abs(x - other.x) <= tolerance
        && Abs(y - other.y) <= tolerance
        && Abs(z - other.z) <= tolerance;
}

template <traits::FloatingType T>
void Vector3Impl<T>::GetOrthogonalAxes(Vector3Impl& out_tangent, Vector3Impl& out_bitangent) const
{
    const T threshold = static_cast<T>(0.99);

    if (Abs(x) < threshold) // X축(Right)과의 내적(x 성분)을 바로 검사하여 최적화
    {
        out_tangent = Cross(Right()).GetNormalized();
    }
    else
    {
        out_tangent = Cross(Up()).GetNormalized();
    }
    out_bitangent = Cross(out_tangent).GetNormalized();
}
} // namespace se::math
