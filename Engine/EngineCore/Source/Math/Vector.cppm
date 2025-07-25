export module SimpleEngine.Math:Vector;
import :MathUtility;

import SimpleEngine.TypeTraits;
import SimpleEngine.Types;
import std;

using namespace se::type_traits;
using namespace se::math;


template <FloatingType T>
struct VectorImpl
{
    T x, y, z;

public:
    constexpr VectorImpl();
    constexpr VectorImpl(T in_x, T in_y, T in_z);
    explicit constexpr VectorImpl(T scalar);

public:
    /** Vector(0, 0, 0) */
    [[nodiscard]] static constexpr VectorImpl Zero();

    /** Vector(1, 1, 1) */
    [[nodiscard]] static constexpr VectorImpl One();

    /** Unit Vector(1, 0, 0) */
    [[nodiscard]] static constexpr VectorImpl UnitX();

    /** Unit Vector(0, 1, 0) */
    [[nodiscard]] static constexpr VectorImpl UnitY();

    /** Unit Vector(0, 0, 1) */
    [[nodiscard]] static constexpr VectorImpl UnitZ();

    /** Unit Vector(0, 0, 1) */
    [[nodiscard]] static constexpr VectorImpl Up();

    /** Unit Vector(0, 1, 0) */
    [[nodiscard]] static constexpr VectorImpl Forward();

    /** Unit Vector(1, 0, 0) */
    [[nodiscard]] static constexpr VectorImpl Right();

public:
    /** Dot Product */
    [[nodiscard]] constexpr T operator|(const VectorImpl& other) const;
    [[nodiscard]] constexpr T Dot(const VectorImpl& other) const;
    [[nodiscard]] static constexpr T DotProduct(const VectorImpl& a, const VectorImpl& b);

    /** Cross Product */
    [[nodiscard]] constexpr VectorImpl operator^(const VectorImpl& other) const;
    [[nodiscard]] constexpr VectorImpl Cross(const VectorImpl& other) const;
    [[nodiscard]] static constexpr VectorImpl CrossProduct(const VectorImpl& a, const VectorImpl& b);

    [[nodiscard]] constexpr T Dist(const VectorImpl& other);
    [[nodiscard]] static constexpr T Distance(const VectorImpl& a, const VectorImpl& b);
    [[nodiscard]] static constexpr T Distance2D(const VectorImpl& a, const VectorImpl& b);

    [[nodiscard]] static constexpr T DistSquared(const VectorImpl& a, const VectorImpl& b);
    [[nodiscard]] static constexpr T DistSquared2D(const VectorImpl& a, const VectorImpl& b);

    [[nodiscard]] constexpr VectorImpl operator+(const VectorImpl& other) const;
    [[nodiscard]] constexpr VectorImpl operator+(T scalar) const;
    [[nodiscard]] friend constexpr VectorImpl operator+(T scalar, const VectorImpl& self);
    constexpr VectorImpl& operator+=(const VectorImpl& other);
    constexpr VectorImpl& operator+=(T scalar);

    [[nodiscard]] constexpr VectorImpl operator-(const VectorImpl& other) const;
    [[nodiscard]] constexpr VectorImpl operator-(T scalar) const;
    [[nodiscard]] friend constexpr VectorImpl operator-(T scalar, const VectorImpl& self);
    constexpr VectorImpl& operator-=(const VectorImpl& other);
    constexpr VectorImpl& operator-=(T scalar);

    [[nodiscard]] constexpr VectorImpl operator*(const VectorImpl& other) const;
    [[nodiscard]] constexpr VectorImpl operator*(T scalar) const;
    [[nodiscard]] friend constexpr VectorImpl operator*(T scalar, const VectorImpl& self);
    constexpr VectorImpl& operator*=(const VectorImpl& other);
    constexpr VectorImpl& operator*=(T scalar);

    [[nodiscard]] constexpr VectorImpl operator/(const VectorImpl& other) const;
    [[nodiscard]] constexpr VectorImpl operator/(T scalar) const;
    [[nodiscard]] friend constexpr VectorImpl operator/(T scalar, const VectorImpl& self);
    constexpr VectorImpl& operator/=(const VectorImpl& other);
    constexpr VectorImpl& operator/=(T scalar);

    [[nodiscard]] constexpr VectorImpl operator-() const;

    [[nodiscard]] constexpr bool operator==(const VectorImpl& other) const;
    [[nodiscard]] constexpr bool operator!=(const VectorImpl& other) const;

public:
    [[nodiscard]] constexpr T Length() const;
    [[nodiscard]] constexpr T SquaredLength() const;

    constexpr void Normalize();
    [[nodiscard]] constexpr VectorImpl GetNormalized() const;
    [[nodiscard]] constexpr bool IsNormalized() const;

    [[nodiscard]] bool IsNearlyZero(T tolerance = KINDA_SMALL_NUMBER) const;
};


template <FloatingType T>
constexpr VectorImpl<T>::VectorImpl()
    : x(0), y(0), z(0)
{
}

template <FloatingType T>
constexpr VectorImpl<T>::VectorImpl(T in_x, T in_y, T in_z)
    : x(in_x), y(in_y), z(in_z)
{
}

template <FloatingType T>
constexpr VectorImpl<T>::VectorImpl(T scalar)
    : x(scalar), y(scalar), z(scalar)
{
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Zero()
{
    return {};
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::One()
{
    return { 1 };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::UnitX()
{
    return { 1, 0, 0 };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::UnitY()
{
    return { 0, 1, 0 };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::UnitZ()
{
    return { 0, 0, 1 };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Up()
{
    return UnitZ();
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Forward()
{
    return UnitY();
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Right()
{
    return UnitX();
}

template <FloatingType T>
constexpr T VectorImpl<T>::operator|(const VectorImpl& other) const
{
    return x * other.x + y * other.y + z * other.z;
}

template <FloatingType T>
constexpr T VectorImpl<T>::Dot(const VectorImpl& other) const
{
    return *this | other;
}

template <FloatingType T>
constexpr T VectorImpl<T>::DotProduct(const VectorImpl& a, const VectorImpl& b)
{
    return a | b;
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator^(const VectorImpl& other) const
{
    return {
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Cross(const VectorImpl& other) const
{
    return *this ^ other;
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::CrossProduct(const VectorImpl& a, const VectorImpl& b)
{
    return a ^ b;
}

template <FloatingType T>
constexpr T VectorImpl<T>::Dist(const VectorImpl& other)
{
    return Distance(*this, other);
}

template <FloatingType T>
constexpr T VectorImpl<T>::Distance(const VectorImpl& a, const VectorImpl& b)
{
    return MathUtils::Sqrt(DistSquared(a, b));
}

template <FloatingType T>
constexpr T VectorImpl<T>::Distance2D(const VectorImpl& a, const VectorImpl& b)
{
    return MathUtils::Sqrt(DistSquared2D(a, b));
}

template <FloatingType T>
constexpr T VectorImpl<T>::DistSquared(const VectorImpl& a, const VectorImpl& b)
{
    return MathUtils::Square(b.x - a.x)
        + MathUtils::Square(b.y - a.y)
        + MathUtils::Square(b.z - a.z);
}

template <FloatingType T>
constexpr T VectorImpl<T>::DistSquared2D(const VectorImpl& a, const VectorImpl& b)
{
    return MathUtils::Square(b.x - a.x)
        + MathUtils::Square(b.y - a.y);
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator+(const VectorImpl& other) const
{
    return { x + other.x, y + other.y, z + other.z };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator+(T scalar) const
{
    return { x + scalar, y + scalar, z + scalar };
}

template <FloatingType T>
constexpr VectorImpl<T> operator+(T scalar, const VectorImpl<T>& self)
{
    return self + scalar;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator+=(const VectorImpl& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator+=(T scalar)
{
    x += scalar;
    y += scalar;
    z += scalar;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator-(const VectorImpl& other) const
{
    return { x - other.x, y - other.y, z - other.z };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator-(T scalar) const
{
    return { x - scalar, y - scalar, z - scalar };
}

template <FloatingType T>
constexpr VectorImpl<T> operator-(T scalar, const VectorImpl<T>& self)
{
    return self - scalar;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator-=(const VectorImpl& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator-=(T scalar)
{
    x -= scalar;
    y -= scalar;
    z -= scalar;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator*(const VectorImpl& other) const
{
    return { x * other.x, y * other.y, z * other.z };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator*(T scalar) const
{
    return { x * scalar, y * scalar, z * scalar };
}

template <FloatingType T>
constexpr VectorImpl<T> operator*(T scalar, const VectorImpl<T>& self)
{
    return self * scalar;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator*=(const VectorImpl& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator/(const VectorImpl& other) const
{
    return { x / other.x, y / other.y, z / other.z };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator/(T scalar) const
{
    return { x / scalar, y / scalar, z / scalar };
}

template <FloatingType T>
constexpr VectorImpl<T> operator/(T scalar, const VectorImpl<T>& self)
{
    return self / scalar;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator/=(const VectorImpl& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T>& VectorImpl<T>::operator/=(T scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::operator-() const
{
    return { -x, -y, -z };
}

template <FloatingType T>
constexpr bool VectorImpl<T>::operator==(const VectorImpl& other) const
{
    return x == other.x && y == other.y && z == other.z;
}

template <FloatingType T>
constexpr bool VectorImpl<T>::operator!=(const VectorImpl& other) const
{
    return !(*this == other);
}

template <FloatingType T>
constexpr T VectorImpl<T>::Length() const
{
    return MathUtils::Sqrt(SquaredLength());
}

template <FloatingType T>
constexpr T VectorImpl<T>::SquaredLength() const
{
    return MathUtils::Square(x)
        + MathUtils::Square(y)
        + MathUtils::Square(z);
}

template <FloatingType T>
constexpr void VectorImpl<T>::Normalize()
{
    T len = Length();
    if (len > std::numeric_limits<T>::epsilon())
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
constexpr VectorImpl<T> VectorImpl<T>::GetNormalized() const
{
    VectorImpl copied = *this;
    copied.Normalize();
    return copied;
}

template <FloatingType T>
constexpr bool VectorImpl<T>::IsNormalized() const
{
    constexpr T epsilon = std::numeric_limits<T>::epsilon();
    return MathUtils::Abs(1.f - SquaredLength()) < epsilon;
}

template <FloatingType T>
bool VectorImpl<T>::IsNearlyZero(T tolerance) const
{
    return MathUtils::Abs(x) <= tolerance
        && MathUtils::Abs(y) <= tolerance
        && MathUtils::Abs(z) <= tolerance;
}
