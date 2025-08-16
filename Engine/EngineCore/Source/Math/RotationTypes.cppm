export module SimpleEngine.Math:RotationTypes;
import :MathUtility;
import :MathLiterals;

import SimpleEngine.Traits;
import SimpleEngine.Types;
import std;

using namespace se::traits::type_traits;


namespace se::math
{
// forward declaration
template <FloatingType>
struct RotatorImpl;


template <FloatingType T>
struct alignas(16) QuaternionImpl
{
    T x, y, z, w;

public:
    constexpr QuaternionImpl();
    constexpr QuaternionImpl(T in_x, T in_y, T in_z, T in_w);
    explicit constexpr QuaternionImpl(const RotatorImpl<T>& rotator);

public:
    /** Quaternion(0, 0, 0, 1) */
    [[nodiscard]] static constexpr QuaternionImpl Identity();

public:
    [[nodiscard]] constexpr QuaternionImpl operator*(const QuaternionImpl& other) const;
    [[nodiscard]] constexpr QuaternionImpl operator*(T scalar) const;

public:
    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr QuaternionImpl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] constexpr bool IsNearlyEqual(const QuaternionImpl& other, T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] constexpr RotatorImpl<T> ToRotator() const;
};

template <FloatingType T>
struct RotatorImpl
{
    Degree<T> pitch; // X axis
    Degree<T> yaw;   // Z axis
    Degree<T> roll;  // Y axis

public:
    constexpr RotatorImpl();
    constexpr RotatorImpl(Degree<T> in_pitch, Degree<T> in_yaw, Degree<T> in_roll);
    explicit constexpr RotatorImpl(const QuaternionImpl<T>& quaternion);

public:
    [[nodiscard]] constexpr QuaternionImpl<T> ToQuaternion() const;
};


//~ Begin QuaternionImpl

template <FloatingType T>
constexpr QuaternionImpl<T>::QuaternionImpl()
    : QuaternionImpl(Identity())
{
}

template <FloatingType T>
constexpr QuaternionImpl<T>::QuaternionImpl(T in_x, T in_y, T in_z, T in_w)
    : x(in_x), y(in_y), z(in_z), w(in_w)
{
}

template <FloatingType T>
constexpr QuaternionImpl<T>::QuaternionImpl(const RotatorImpl<T>& rotator)
{
    const Radian<T> pitch_rad{ rotator.pitch };
    const Radian<T> yaw_rad{ rotator.yaw };
    const Radian<T> roll_rad{ rotator.roll };

    const T sin_p = MathUtils::Sin(pitch_rad * 0.5), cos_p = MathUtils::Cos(pitch_rad * 0.5);
    const T sin_y = MathUtils::Sin(yaw_rad * 0.5), cos_y = MathUtils::Cos(yaw_rad * 0.5);
    const T sin_r = MathUtils::Sin(roll_rad * 0.5), cos_r = MathUtils::Cos(roll_rad * 0.5);

    // Yaw * Pitch * Roll
    x = sin_r * cos_p * cos_y - cos_r * sin_p * sin_y;
    y = cos_r * sin_p * cos_y + sin_r * cos_p * sin_y;
    z = cos_r * cos_p * sin_y - sin_r * sin_p * cos_y;
    w = cos_r * cos_p * cos_y + sin_r * sin_p * sin_y;
}

template <FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::Identity()
{
    return { 0, 0, 0, 1 };
}

template <FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::operator*(const QuaternionImpl& other) const
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
constexpr QuaternionImpl<T> QuaternionImpl<T>::operator*(T scalar) const
{
    return { x * scalar, y * scalar, z * scalar, w * scalar };
}

template <FloatingType T>
constexpr void QuaternionImpl<T>::Normalize(T tolerance)
{
    const T square_sum = x * x + y * y + z * z + w * w;
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
constexpr QuaternionImpl<T> QuaternionImpl<T>::GetNormalized(T tolerance) const
{
    QuaternionImpl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <FloatingType T>
constexpr bool QuaternionImpl<T>::IsNormalized(T tolerance) const
{
    return MathUtils::Abs(x * x + y * y + z * z + w * w - 1.0f) < tolerance;
}

template <FloatingType T>
constexpr bool QuaternionImpl<T>::IsNearlyEqual(const QuaternionImpl& other, T tolerance) const
{
    return (
        MathUtils::Abs(x - other.x) <= tolerance && MathUtils::Abs(y - other.y) <= tolerance
        && MathUtils::Abs(z - other.z) <= tolerance && MathUtils::Abs(w - other.w) <= tolerance
    ) || (
        MathUtils::Abs(x + other.x) <= tolerance && MathUtils::Abs(y + other.y) <= tolerance
        && MathUtils::Abs(z + other.z) <= tolerance && MathUtils::Abs(w + other.w) <= tolerance
    );
}

template <FloatingType T>
constexpr RotatorImpl<T> QuaternionImpl<T>::ToRotator() const
{
    return { *this };
}

//~ End QuaternionImpl

//~ Begin RotatorImpl

template <FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl()
    : pitch(0), yaw(0), roll(0)
{
}

template <FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl(Degree<T> in_pitch, Degree<T> in_yaw, Degree<T> in_roll)
    : pitch(in_pitch), yaw(in_yaw), roll(in_roll)
{
}

template <FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl(const QuaternionImpl<T>& quaternion)
{
    const T sinr_cosp = 2 * (quaternion.w * quaternion.x + quaternion.y * quaternion.z);
    const T cosr_cosp = 1 - 2 * (quaternion.x * quaternion.x + quaternion.y * quaternion.y);
    const Radian<T> roll_rad = MathUtils::Atan2(sinr_cosp, cosr_cosp);

    const T sinp = 2 * (quaternion.w * quaternion.y - quaternion.z * quaternion.x);
    Radian<T> pitch_rad{ 0 };
    if (MathUtils::Abs(sinp) >= 1)
    {
        pitch_rad.value = MathUtils::CopySign(std::numbers::pi_v<T> / 2, sinp);
    }
    else
    {
        pitch_rad = MathUtils::Asin(sinp);
    }

    const T siny_cosp = 2 * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
    const T cosy_cosp = 1 - 2 * (quaternion.y * quaternion.y + quaternion.z * quaternion.z);
    const Radian<T> yaw_rad(MathUtils::Atan2(siny_cosp, cosy_cosp));

    pitch = Degree<T>{ pitch_rad };
    yaw = Degree<T>{ yaw_rad };
    roll = Degree<T>{ roll_rad };
}

template <FloatingType T>
constexpr QuaternionImpl<T> RotatorImpl<T>::ToQuaternion() const
{
    return { *this };
}

//~ End RotatorImpl
}
