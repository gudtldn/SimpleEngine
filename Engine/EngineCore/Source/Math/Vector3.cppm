export module SE.Math:Vector3;
import :MathUtility;

import SE.Traits;
import SE.Types;
import std;

import <cassert>;

using namespace se::traits::type_traits;


namespace se::math
{
template <FloatingType T>
struct Vector3Impl
{
    T x, y, z;

public:
    constexpr Vector3Impl() = default;
    constexpr Vector3Impl(T in_x, T in_y, T in_z);
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
    [[nodiscard]] friend constexpr Vector3Impl operator+(T scalar, const Vector3Impl& self);
    constexpr Vector3Impl& operator+=(const Vector3Impl& other);
    constexpr Vector3Impl& operator+=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator-(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator-(T scalar) const;
    constexpr Vector3Impl& operator-=(const Vector3Impl& other);
    constexpr Vector3Impl& operator-=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator*(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator*(T scalar) const;
    [[nodiscard]] friend constexpr Vector3Impl operator*(T scalar, const Vector3Impl& self);
    constexpr Vector3Impl& operator*=(const Vector3Impl& other);
    constexpr Vector3Impl& operator*=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator/(const Vector3Impl& other) const;
    [[nodiscard]] constexpr Vector3Impl operator/(T scalar) const;
    constexpr Vector3Impl& operator/=(const Vector3Impl& other);
    constexpr Vector3Impl& operator/=(T scalar);

    [[nodiscard]] constexpr Vector3Impl operator-() const;

    [[nodiscard]] constexpr bool operator==(const Vector3Impl& other) const;
    [[nodiscard]] constexpr bool operator!=(const Vector3Impl& other) const;

    [[nodiscard]] constexpr T operator[](size_t index);
    [[nodiscard]] constexpr T operator[](size_t index) const;

public:
    [[nodiscard]] constexpr T Length() const;
    [[nodiscard]] constexpr T SquaredLength() const;

    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr Vector3Impl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] bool IsNearlyZero(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] bool IsNearlyEqual(const Vector3Impl& other, T tolerance = KINDA_SMALL_NUMBER) const;
};


template <FloatingType T>
constexpr Vector3Impl<T>::Vector3Impl(T in_x, T in_y, T in_z)
    : x(in_x), y(in_y), z(in_z)
{
}

template <FloatingType T>
constexpr Vector3Impl<T>::Vector3Impl(T scalar)
    : x(scalar), y(scalar), z(scalar)
{
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Zero()
{
    return Vector3Impl{ 0 };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::One()
{
    return Vector3Impl{ 1 };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::UnitX()
{
    return Vector3Impl{ 1, 0, 0 };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::UnitY()
{
    return Vector3Impl{ 0, 1, 0 };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::UnitZ()
{
    return Vector3Impl{ 0, 0, 1 };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Up()
{
    return UnitZ();
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Forward()
{
    return UnitY();
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Right()
{
    return UnitX();
}

template <FloatingType T>
constexpr T Vector3Impl<T>::operator|(const Vector3Impl& other) const
{
    return x * other.x + y * other.y + z * other.z;
}

template <FloatingType T>
constexpr T Vector3Impl<T>::Dot(const Vector3Impl& other) const
{
    return *this | other;
}

template <FloatingType T>
constexpr T Vector3Impl<T>::DotProduct(const Vector3Impl& a, const Vector3Impl& b)
{
    return a | b;
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator^(const Vector3Impl& other) const
{
    return Vector3Impl{
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::Cross(const Vector3Impl& other) const
{
    return *this ^ other;
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::CrossProduct(const Vector3Impl& a, const Vector3Impl& b)
{
    return a ^ b;
}

template <FloatingType T>
constexpr T Vector3Impl<T>::Dist(const Vector3Impl& other)
{
    return Distance(*this, other);
}

template <FloatingType T>
constexpr T Vector3Impl<T>::Distance(const Vector3Impl& a, const Vector3Impl& b)
{
    return MathUtility::Sqrt(DistSquared(a, b));
}

template <FloatingType T>
constexpr T Vector3Impl<T>::Distance2D(const Vector3Impl& a, const Vector3Impl& b)
{
    return MathUtility::Sqrt(DistSquared2D(a, b));
}

template <FloatingType T>
constexpr T Vector3Impl<T>::DistSquared(const Vector3Impl& a, const Vector3Impl& b)
{
    return MathUtility::Square(b.x - a.x)
        + MathUtility::Square(b.y - a.y)
        + MathUtility::Square(b.z - a.z);
}

template <FloatingType T>
constexpr T Vector3Impl<T>::DistSquared2D(const Vector3Impl& a, const Vector3Impl& b)
{
    return MathUtility::Square(b.x - a.x)
        + MathUtility::Square(b.y - a.y);
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator+(const Vector3Impl& other) const
{
    return Vector3Impl{ x + other.x, y + other.y, z + other.z };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator+(T scalar) const
{
    return Vector3Impl{ x + scalar, y + scalar, z + scalar };
}

template <FloatingType T>
constexpr Vector3Impl<T> operator+(T scalar, const Vector3Impl<T>& self)
{
    return self + scalar;
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator+=(const Vector3Impl& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator+=(T scalar)
{
    x += scalar;
    y += scalar;
    z += scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator-(const Vector3Impl& other) const
{
    return Vector3Impl{ x - other.x, y - other.y, z - other.z };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator-(T scalar) const
{
    return Vector3Impl{ x - scalar, y - scalar, z - scalar };
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator-=(const Vector3Impl& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator-=(T scalar)
{
    x -= scalar;
    y -= scalar;
    z -= scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator*(const Vector3Impl& other) const
{
    return Vector3Impl{ x * other.x, y * other.y, z * other.z };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator*(T scalar) const
{
    return Vector3Impl{ x * scalar, y * scalar, z * scalar };
}

template <FloatingType T>
constexpr Vector3Impl<T> operator*(T scalar, const Vector3Impl<T>& self)
{
    return self * scalar;
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator*=(const Vector3Impl& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator/(const Vector3Impl& other) const
{
    return Vector3Impl{ x / other.x, y / other.y, z / other.z };
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator/(T scalar) const
{
    return Vector3Impl{ x / scalar, y / scalar, z / scalar };
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator/=(const Vector3Impl& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T>& Vector3Impl<T>::operator/=(T scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::operator-() const
{
    return Vector3Impl{ -x, -y, -z };
}

template <FloatingType T>
constexpr bool Vector3Impl<T>::operator==(const Vector3Impl& other) const
{
    return x == other.x && y == other.y && z == other.z;
}

template <FloatingType T>
constexpr bool Vector3Impl<T>::operator!=(const Vector3Impl& other) const
{
    return !(*this == other);
}

template <FloatingType T>
constexpr T Vector3Impl<T>::operator[](size_t index)
{
    assert(index < 3);
    return (&x)[index];
}

template <FloatingType T>
constexpr T Vector3Impl<T>::operator[](size_t index) const
{
    assert(index < 3);
    return (&x)[index];
}

template <FloatingType T>
constexpr T Vector3Impl<T>::Length() const
{
    return MathUtility::Sqrt(SquaredLength());
}

template <FloatingType T>
constexpr T Vector3Impl<T>::SquaredLength() const
{
    return MathUtility::Square(x)
        + MathUtility::Square(y)
        + MathUtility::Square(z);
}

template <FloatingType T>
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

template <FloatingType T>
constexpr Vector3Impl<T> Vector3Impl<T>::GetNormalized(T tolerance) const
{
    Vector3Impl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <FloatingType T>
constexpr bool Vector3Impl<T>::IsNormalized(T tolerance) const
{
    return MathUtility::Abs(1.f - SquaredLength()) < tolerance;
}

template <FloatingType T>
bool Vector3Impl<T>::IsNearlyZero(T tolerance) const
{
    return MathUtility::Abs(x) <= tolerance
        && MathUtility::Abs(y) <= tolerance
        && MathUtility::Abs(z) <= tolerance;
}

template <FloatingType T>
bool Vector3Impl<T>::IsNearlyEqual(const Vector3Impl& other, T tolerance) const
{
    return MathUtility::Abs(x - other.x) <= tolerance
        && MathUtility::Abs(y - other.y) <= tolerance
        && MathUtility::Abs(z - other.z) <= tolerance;
}
}
