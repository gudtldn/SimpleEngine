export module SimpleEngine.Math:Vector;

import SimpleEngine.TypeTraits;
import SimpleEngine.Types;
import std;

using namespace se::type_traits;


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
    static constexpr VectorImpl Zero();

    /** Vector(1, 1, 1) */
    static constexpr VectorImpl One();

    /** Unit Vector(0, 0, 1) */
    static constexpr VectorImpl Up();

    /** Unit Vector(0, 1, 0) */
    static constexpr VectorImpl Forward();

    /** Unit Vector(1, 0, 0) */
    static constexpr VectorImpl Right();

public:
    /** Dot Product */
    constexpr T operator|(const VectorImpl& other) const;
    constexpr T Dot(const VectorImpl& other) const;
    static constexpr T DotProduct(const VectorImpl& a, const VectorImpl& b);

    /** Cross Product */
    constexpr VectorImpl operator^(const VectorImpl& other) const;
    constexpr VectorImpl Cross(const VectorImpl& other) const;
    static constexpr VectorImpl CrossProduct(const VectorImpl& a, const VectorImpl& b);

    constexpr T Dist(const VectorImpl& other);
    static constexpr T Distance(const VectorImpl& a, const VectorImpl& b);

    constexpr VectorImpl operator+(const VectorImpl& other) const;
    constexpr VectorImpl operator+(T scalar) const;
    constexpr VectorImpl& operator+=(const VectorImpl& other);

    constexpr VectorImpl operator-(const VectorImpl& other) const;
    constexpr VectorImpl operator-(T scalar) const;
    constexpr VectorImpl& operator-=(const VectorImpl& other);

    constexpr VectorImpl operator*(const VectorImpl& other) const;
    constexpr VectorImpl operator*(T scalar) const;
    constexpr VectorImpl& operator*=(const VectorImpl& other);
    constexpr VectorImpl& operator*=(T scalar);

    constexpr VectorImpl operator/(const VectorImpl& other) const;
    constexpr VectorImpl operator/(T scalar) const;
    constexpr VectorImpl& operator/=(T scalar);

    constexpr VectorImpl operator-() const;

    constexpr bool operator==(const VectorImpl& other) const;
    constexpr bool operator!=(const VectorImpl& other) const;

public:
    constexpr T Length() const;
    constexpr T SquaredLength() const;
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
constexpr VectorImpl<T> VectorImpl<T>::Up()
{
    return { 0, 0, 1 };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Forward()
{
    return { 0, 1, 0 };
}

template <FloatingType T>
constexpr VectorImpl<T> VectorImpl<T>::Right()
{
    return { 1, 0, 0 };
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
constexpr VectorImpl<T>& VectorImpl<T>::operator+=(const VectorImpl& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
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
constexpr VectorImpl<T>& VectorImpl<T>::operator-=(const VectorImpl& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
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
