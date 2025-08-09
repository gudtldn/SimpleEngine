export module SimpleEngine.Math:Quat;
import :MathUtility;

import SimpleEngine.Traits;
import SimpleEngine.Types;
import std;

using namespace se::traits::type_traits;
using namespace se::math;


template <FloatingType T>
struct alignas(16) QuatImpl
{
    T x, y, z, w;

public:
    constexpr QuatImpl();
    constexpr QuatImpl(T in_x, T in_y, T in_z, T in_w);

public:
    /** Quat(0, 0, 0, 1) */
    [[nodiscard]] static constexpr QuatImpl Identity();

public:
    [[nodiscard]] constexpr QuatImpl operator*(const QuatImpl& other) const;
    [[nodiscard]] constexpr QuatImpl operator*(T scalar) const;

public:
    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr QuatImpl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] constexpr bool IsNearlyEqual(const QuatImpl& other, T tolerance = KINDA_SMALL_NUMBER) const;
};

template <FloatingType T>
constexpr QuatImpl<T>::QuatImpl()
    : QuatImpl(Identity())
{
}

template <FloatingType T>
constexpr QuatImpl<T>::QuatImpl(T in_x, T in_y, T in_z, T in_w)
    : x(in_x), y(in_y), z(in_z), w(in_w)
{
}

template <FloatingType T>
constexpr QuatImpl<T> QuatImpl<T>::Identity()
{
    return { 0, 0, 0, 1 };
}

template <FloatingType T>
constexpr QuatImpl<T> QuatImpl<T>::operator*(const QuatImpl& other) const
{
    // (Q1 * Q2).X = (W1*X2 + X1*W2 + Y1*Z2 - Z1*Y2)
    // (Q1 * Q2).Y = (W1*Y2 - X1*Z2 + Y1*W2 + Z1*X2)
    // (Q1 * Q2).Z = (W1*Z2 + X1*Y2 - Y1*X2 + Z1*W2)
    // (Q1 * Q2).W = (W1*W2 - X1*X2 - Y1*Y2 - Z1*Z2)
    return {
        w * other.x + x * other.w + y * other.z - z * other.y, // New X
        w * other.y - x * other.z + y * other.w + z * other.x, // New Y
        w * other.z + x * other.y - y * other.x + z * other.w, // New Z
        w * other.w - x * other.x - y * other.y - z * other.z  // New W
    };
}

template <FloatingType T>
constexpr QuatImpl<T> QuatImpl<T>::operator*(T scalar) const
{
    return { x * scalar, y * scalar, z * scalar, w * scalar };
}

template <FloatingType T>
constexpr void QuatImpl<T>::Normalize(T tolerance)
{
    const T square_sum = x*x + y*y + z*z + w*w;
    if (square_sum >= tolerance)
    {
        const T scale = MathUtils::InvSqrt(square_sum);
        x *= scale;
        y *= scale;
        z *= scale;
        w *= scale;
    }
    else
    {
        *this = Identity();
    }
}

template <FloatingType T>
constexpr QuatImpl<T> QuatImpl<T>::GetNormalized(T tolerance) const
{
    QuatImpl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <FloatingType T>
constexpr bool QuatImpl<T>::IsNormalized(T tolerance) const
{
    return MathUtils::Abs(x*x + y*y + z*z + w*w - 1.0f) < tolerance;
}

template <FloatingType T>
constexpr bool QuatImpl<T>::IsNearlyEqual(const QuatImpl& other, T tolerance) const
{
    return (MathUtils::Abs(x - other.x) <= tolerance && MathUtils::Abs(y - other.y) <= tolerance && MathUtils::Abs(z - other.z) <= tolerance && MathUtils::Abs(w - other.w) <= tolerance)
        || (MathUtils::Abs(x + other.x) <= tolerance && MathUtils::Abs(y + other.y) <= tolerance && MathUtils::Abs(z + other.z) <= tolerance && MathUtils::Abs(w + other.w) <= tolerance);
}
