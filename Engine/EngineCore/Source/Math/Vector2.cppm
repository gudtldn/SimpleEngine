export module SE.Math:Vector2;
import :MathUtility;

import SE.Traits;
import SE.Types;
import std;

import <cassert>;

using namespace se::traits::type_traits;


namespace se::math
{
template <FloatingType T>
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
    [[nodiscard]] friend constexpr Vector2Impl operator+(T scalar, const Vector2Impl& self);
    constexpr Vector2Impl& operator+=(const Vector2Impl& other);
    constexpr Vector2Impl& operator+=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator-(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator-(T scalar) const;
    constexpr Vector2Impl& operator-=(const Vector2Impl& other);
    constexpr Vector2Impl& operator-=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator*(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator*(T scalar) const;
    [[nodiscard]] friend constexpr Vector2Impl operator*(T scalar, const Vector2Impl& self);
    constexpr Vector2Impl& operator*=(const Vector2Impl& other);
    constexpr Vector2Impl& operator*=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator/(const Vector2Impl& other) const;
    [[nodiscard]] constexpr Vector2Impl operator/(T scalar) const;
    constexpr Vector2Impl& operator/=(const Vector2Impl& other);
    constexpr Vector2Impl& operator/=(T scalar);

    [[nodiscard]] constexpr Vector2Impl operator-() const;

    [[nodiscard]] constexpr bool operator==(const Vector2Impl& other) const;
    [[nodiscard]] constexpr bool operator!=(const Vector2Impl& other) const;

    [[nodiscard]] constexpr T operator[](size_t index);
    [[nodiscard]] constexpr T operator[](size_t index) const;

public:
    [[nodiscard]] constexpr T Length() const;
    [[nodiscard]] constexpr T SquaredLength() const;

    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr Vector2Impl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] bool IsNearlyZero(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] bool IsNearlyEqual(const Vector2Impl& other, T tolerance = KINDA_SMALL_NUMBER) const;
};


template <FloatingType T>
constexpr Vector2Impl<T>::Vector2Impl(T in_x, T in_y)
    : x(in_x)
    , y(in_y)
{
}

template <FloatingType T>
constexpr Vector2Impl<T>::Vector2Impl(T scalar)
    : x(scalar)
    , y(scalar)
{
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::Zero()
{
    return Vector2Impl{};
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::One()
{
    return Vector2Impl{ 1 };
}

template <FloatingType T>
constexpr T Vector2Impl<T>::operator|(const Vector2Impl& other) const
{
    return x * other.x + y * other.y;
}

template <FloatingType T>
constexpr T Vector2Impl<T>::Dot(const Vector2Impl& other) const
{
    return *this | other;
}

template <FloatingType T>
constexpr T Vector2Impl<T>::DotProduct(const Vector2Impl& a, const Vector2Impl& b)
{
    return a | b;
}

template <FloatingType T>
constexpr T Vector2Impl<T>::Dist(const Vector2Impl& other)
{
    return Distance(*this, other);
}

template <FloatingType T>
constexpr T Vector2Impl<T>::Distance(const Vector2Impl& a, const Vector2Impl& b)
{
    return MathUtility::Sqrt(DistSquared(a, b));
}

template <FloatingType T>
constexpr T Vector2Impl<T>::DistSquared(const Vector2Impl& a, const Vector2Impl& b)
{
    return MathUtility::Square(b.x - a.x)
        + MathUtility::Square(b.y - a.y);
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator+(const Vector2Impl& other) const
{
    return Vector2Impl{ x + other.x, y + other.y };
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator+(T scalar) const
{
    return Vector2Impl{ x + scalar, y + scalar };
}

template <FloatingType T>
constexpr Vector2Impl<T> operator+(T scalar, const Vector2Impl<T>& self)
{
    return self + scalar;
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator+=(const Vector2Impl& other)
{
    x += other.x;
    y += other.y;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator+=(T scalar)
{
    x += scalar;
    y += scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator-(const Vector2Impl& other) const
{
    return Vector2Impl{ x - other.x, y - other.y };
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator-(T scalar) const
{
    return Vector2Impl{ x - scalar, y - scalar };
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator-=(const Vector2Impl& other)
{
    x -= other.x;
    y -= other.y;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator-=(T scalar)
{
    x -= scalar;
    y -= scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator*(const Vector2Impl& other) const
{
    return Vector2Impl{ x * other.x, y * other.y };
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator*(T scalar) const
{
    return Vector2Impl{ x * scalar, y * scalar };
}

template <FloatingType T>
constexpr Vector2Impl<T> operator*(T scalar, const Vector2Impl<T>& self)
{
    return self * scalar;
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator*=(const Vector2Impl& other)
{
    x *= other.x;
    y *= other.y;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator/(const Vector2Impl& other) const
{
    return Vector2Impl{ x / other.x, y / other.y };
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator/(T scalar) const
{
    return Vector2Impl{ x / scalar, y / scalar };
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator/=(const Vector2Impl& other)
{
    x /= other.x;
    y /= other.y;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T>& Vector2Impl<T>::operator/=(T scalar)
{
    x /= scalar;
    y /= scalar;
    return *this;
}

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::operator-() const
{
    return Vector2Impl{ -x, -y };
}

template <FloatingType T>
constexpr bool Vector2Impl<T>::operator==(const Vector2Impl& other) const
{
    return x == other.x && y == other.y;
}

template <FloatingType T>
constexpr bool Vector2Impl<T>::operator!=(const Vector2Impl& other) const
{
    return !(*this == other);
}

template <FloatingType T>
constexpr T Vector2Impl<T>::operator[](size_t index)
{
    assert(index < 2);
    return (&x)[index];
}

template <FloatingType T>
constexpr T Vector2Impl<T>::operator[](size_t index) const
{
    assert(index < 2);
    return (&x)[index];
}

template <FloatingType T>
constexpr T Vector2Impl<T>::Length() const
{
    return MathUtility::Sqrt(SquaredLength());
}

template <FloatingType T>
constexpr T Vector2Impl<T>::SquaredLength() const
{
    return MathUtility::Square(x)
        + MathUtility::Square(y);
}

template <FloatingType T>
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

template <FloatingType T>
constexpr Vector2Impl<T> Vector2Impl<T>::GetNormalized(T tolerance) const
{
    Vector2Impl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <FloatingType T>
constexpr bool Vector2Impl<T>::IsNormalized(T tolerance) const
{
    return MathUtility::Abs(1.f - SquaredLength()) < tolerance;
}

template <FloatingType T>
bool Vector2Impl<T>::IsNearlyZero(T tolerance) const
{
    return MathUtility::Abs(x) <= tolerance
        && MathUtility::Abs(y) <= tolerance;
}

template <FloatingType T>
bool Vector2Impl<T>::IsNearlyEqual(const Vector2Impl& other, T tolerance) const
{
    return MathUtility::Abs(x - other.x) <= tolerance
        && MathUtility::Abs(y - other.y) <= tolerance;
}
}
