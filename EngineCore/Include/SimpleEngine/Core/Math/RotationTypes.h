#pragma once
#include <cassert>

#include "SimpleEngine/Core/Math/MathFwd.h"
#include "SimpleEngine/Core/Math/MathLiterals.h"
#include "SimpleEngine/Core/Math/MathUtility.h"


namespace se::math
{
template <traits::FloatingType T>
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

    [[nodiscard]] static QuaternionImpl FromAxisAngle(const Vector3Impl<T>& axis, Radian<T> angle);

public:
    [[nodiscard]] constexpr QuaternionImpl operator*(const QuaternionImpl& other) const;
    [[nodiscard]] constexpr QuaternionImpl operator*(T scalar) const;

    QuaternionImpl& operator*=(const QuaternionImpl& other);
    QuaternionImpl& operator*=(T scalar);

public:
    constexpr void Normalize(T tolerance = KINDA_SMALL_NUMBER);
    [[nodiscard]] constexpr QuaternionImpl GetNormalized(T tolerance = KINDA_SMALL_NUMBER) const;
    [[nodiscard]] constexpr bool IsNormalized(T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] constexpr bool IsNearlyEqual(const QuaternionImpl& other, T tolerance = KINDA_SMALL_NUMBER) const;

    [[nodiscard]] constexpr Vector3Impl<T> GetForwardVector() const;
    [[nodiscard]] constexpr Vector3Impl<T> GetRightVector() const;
    [[nodiscard]] constexpr Vector3Impl<T> GetUpVector() const;

    [[nodiscard]] constexpr RotatorImpl<T> ToRotator() const;
};

template <traits::FloatingType T>
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
    /** Rotator(0, 0, 0) */
    [[nodiscard]] static constexpr RotatorImpl ZeroRotator();

public:
    [[nodiscard]] constexpr RotatorImpl operator+(const RotatorImpl& other) const;
    RotatorImpl& operator+=(const RotatorImpl& other);

    [[nodiscard]] constexpr RotatorImpl operator-(const RotatorImpl& other) const;
    RotatorImpl& operator-=(const RotatorImpl& other);

    template <traits::NumberType Num>
    [[nodiscard]] constexpr RotatorImpl operator*(Num scale) const;

    template <traits::NumberType Num>
    RotatorImpl& operator*=(Num scale);

public:
    [[nodiscard]] constexpr Vector3Impl<T> GetForwardVector() const;
    [[nodiscard]] constexpr Vector3Impl<T> GetRightVector() const;
    [[nodiscard]] constexpr Vector3Impl<T> GetUpVector() const;

    [[nodiscard]] constexpr QuaternionImpl<T> ToQuaternion() const;
};


//~ Begin QuaternionImpl

template <traits::FloatingType T>
constexpr QuaternionImpl<T>::QuaternionImpl()
    : x(0), y(0), z(0), w(1)
{
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T>::QuaternionImpl(T in_x, T in_y, T in_z, T in_w)
    : x(in_x), y(in_y), z(in_z), w(in_w)
{
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T>::QuaternionImpl(const RotatorImpl<T>& rotator)
{
    const Radian<T> half_rad_p{ rotator.pitch * static_cast<T>(0.5) };
    const Radian<T> half_rad_y{ rotator.yaw * static_cast<T>(0.5) };
    const Radian<T> half_rad_r{ rotator.roll * static_cast<T>(0.5) };

    const T sp = MathUtility::Sin(half_rad_p), cp = MathUtility::Cos(half_rad_p);
    const T sy = MathUtility::Sin(half_rad_y), cy = MathUtility::Cos(half_rad_y);
    const T sr = MathUtility::Sin(half_rad_r), cr = MathUtility::Cos(half_rad_r);

    // Yaw * Pitch * Roll
    x = cr * sp * cy + sr * cp * sy;
    y = sr * cp * cy - cr * sp * sy;
    z = cr * cp * sy - sr * sp * cy;
    w = cr * cp * cy + sr * sp * sy;
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::Identity()
{
    return QuaternionImpl{};
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::operator*(const QuaternionImpl& other) const
{
    // (Q1 * Q2).X = (W1*X2 + X1*W2 + Y1*Z2 - Z1*Y2)
    // (Q1 * Q2).Y = (W1*Y2 - X1*Z2 + Y1*W2 + Z1*X2)
    // (Q1 * Q2).Z = (W1*Z2 + X1*Y2 - Y1*X2 + Z1*W2)
    // (Q1 * Q2).W = (W1*W2 - X1*X2 - Y1*Y2 - Z1*Z2)
    return QuaternionImpl{
        w * other.x + x * other.w + y * other.z - z * other.y, // New X
        w * other.y - x * other.z + y * other.w + z * other.x, // New Y
        w * other.z + x * other.y - y * other.x + z * other.w, // New Z
        w * other.w - x * other.x - y * other.y - z * other.z  // New W
    };
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::operator*(T scalar) const
{
    return QuaternionImpl{ x * scalar, y * scalar, z * scalar, w * scalar };
}

template <traits::FloatingType T>
constexpr void QuaternionImpl<T>::Normalize(T tolerance)
{
    const T square_sum = x * x + y * y + z * z + w * w;
    if (square_sum >= tolerance)
    {
        const T scale = MathUtility::InvSqrt(square_sum);
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

template <traits::FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::GetNormalized(T tolerance) const
{
    QuaternionImpl copied = *this;
    copied.Normalize(tolerance);
    return copied;
}

template <traits::FloatingType T>
constexpr bool QuaternionImpl<T>::IsNormalized(T tolerance) const
{
    return MathUtility::Abs(x * x + y * y + z * z + w * w - 1.0f) < tolerance;
}

template <traits::FloatingType T>
constexpr bool QuaternionImpl<T>::IsNearlyEqual(const QuaternionImpl& other, T tolerance) const
{
    return (
        MathUtility::Abs(x - other.x) <= tolerance && MathUtility::Abs(y - other.y) <= tolerance
        && MathUtility::Abs(z - other.z) <= tolerance && MathUtility::Abs(w - other.w) <= tolerance
    ) || (
        MathUtility::Abs(x + other.x) <= tolerance && MathUtility::Abs(y + other.y) <= tolerance
        && MathUtility::Abs(z + other.z) <= tolerance && MathUtility::Abs(w + other.w) <= tolerance
    );
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> QuaternionImpl<T>::GetForwardVector() const
{
    const T xx = x * x;
    const T zz = z * z;
    const T xy = x * y;
    const T yz = y * z;
    const T wz = w * z;
    const T wx = w * x;

    return Vector3Impl<T>{
        static_cast<T>(2.0) * (xy - wz),
        static_cast<T>(1.0) - static_cast<T>(2.0) * (xx + zz),
        static_cast<T>(2.0) * (yz + wx)
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> QuaternionImpl<T>::GetRightVector() const
{
    const T yy = y * y;
    const T zz = z * z;
    const T xy = x * y;
    const T xz = x * z;
    const T wz = w * z;
    const T wy = w * y;

    return Vector3Impl<T>{
        static_cast<T>(1.0) - static_cast<T>(2.0) * (yy + zz),
        static_cast<T>(2.0) * (xy + wz),
        static_cast<T>(2.0) * (xz - wy)
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> QuaternionImpl<T>::GetUpVector() const
{
    const T xx = x * x;
    const T yy = y * y;
    const T xz = x * z;
    const T yz = y * z;
    const T wy = w * y;
    const T wx = w * x;

    return Vector3Impl<T>{
        static_cast<T>(2.0) * (xz + wy),
        static_cast<T>(2.0) * (yz - wx),
        static_cast<T>(1.0) - static_cast<T>(2.0) * (xx + yy)
    };
}

template <traits::FloatingType T>
constexpr RotatorImpl<T> QuaternionImpl<T>::ToRotator() const
{
    return RotatorImpl{ *this };
}

template <traits::FloatingType T>
QuaternionImpl<T> QuaternionImpl<T>::FromAxisAngle(const Vector3Impl<T>& axis, Radian<T> angle)
{
    const Radian half_angle = angle * static_cast<T>(0.5);
    const T sin_half_angle = MathUtility::Sin(half_angle);
    const T cos_half_angle = MathUtility::Cos(half_angle);

    assert(axis.IsNormalized());

    return QuaternionImpl{
        axis.x * sin_half_angle,
        axis.y * sin_half_angle,
        axis.z * sin_half_angle,
        cos_half_angle
    };
}

template <traits::FloatingType T>
QuaternionImpl<T>& QuaternionImpl<T>::operator*=(const QuaternionImpl& other)
{
    x = w * other.x + x * other.w + y * other.z - z * other.y;
    y = w * other.y - x * other.z + y * other.w + z * other.x;
    z = w * other.z + x * other.y - y * other.x + z * other.w;
    w = w * other.w - x * other.x - y * other.y - z * other.z;
    return *this;
}

template <traits::FloatingType T>
QuaternionImpl<T>& QuaternionImpl<T>::operator*=(T scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

//~ End QuaternionImpl

//~ Begin RotatorImpl

template <traits::FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl()
    : pitch(0), yaw(0), roll(0)
{
}

template <traits::FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl(Degree<T> in_pitch, Degree<T> in_yaw, Degree<T> in_roll)
    : pitch(in_pitch), yaw(in_yaw), roll(in_roll)
{
}

template <traits::FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl(const QuaternionImpl<T>& quaternion)
{
    // sin(pitch) 값을 추출해서 짐벌 락 상태인지 체크 (±1에 가까우면 짐벌 락)
    const T sin_p = -static_cast<T>(2.0) * (quaternion.y * quaternion.z - quaternion.w * quaternion.x);

    // 짐벌 락 체크 (Pitch가 +/- 90도인 경우)
    if (MathUtility::Abs(sin_p) >= static_cast<T>(1.0 - KINDA_SMALL_NUMBER))
    {
        // Pitch는 90도로 고정
        pitch = Degree<T>{ MathUtility::CopySign(static_cast<T>(90), sin_p) };

        // 이때는 Yaw와 Roll이 같은 동작을 하므로, Roll을 0으로 고정하고 Yaw에 회전을 적용
        yaw = Degree<T>{
            MathUtility::Atan2(quaternion.y, quaternion.w)
            * -MathUtility::CopySign(static_cast<T>(2.0), sin_p)
        };
        roll = Degree<T>{ static_cast<T>(0.0) };
    }
    else
    {
        // Pitch
        pitch = Degree<T>{ MathUtility::Asin(sin_p) };

        // Yaw
        const T tan_y_numerator = static_cast<T>(2.0) * (quaternion.x * quaternion.y + quaternion.w * quaternion.z);
        const T tan_y_denominator = static_cast<T>(1.0) - static_cast<T>(2.0) * (quaternion.x * quaternion.x + quaternion.z * quaternion.z);
        yaw = Degree<T>{ MathUtility::Atan2(tan_y_numerator, tan_y_denominator) };

        // Roll
        const T tan_r_numerator = static_cast<T>(2.0) * (quaternion.x * quaternion.z + quaternion.w * quaternion.y);
        const T tan_r_denominator = static_cast<T>(1.0) - static_cast<T>(2.0) * (quaternion.x * quaternion.x + quaternion.y * quaternion.y);
        roll = Degree<T>{ MathUtility::Atan2(tan_r_numerator, tan_r_denominator) };
    }
}

template <traits::FloatingType T>
constexpr RotatorImpl<T> RotatorImpl<T>::ZeroRotator()
{
    return RotatorImpl{};
}

template <traits::FloatingType T>
constexpr RotatorImpl<T> RotatorImpl<T>::operator+(const RotatorImpl& other) const
{
    return RotatorImpl{
        pitch + other.pitch,
        yaw + other.yaw,
        roll + other.roll
    };
}

template <traits::FloatingType T>
RotatorImpl<T>& RotatorImpl<T>::operator+=(const RotatorImpl& other)
{
    pitch += other.pitch;
    yaw += other.yaw;
    roll += other.roll;
    return *this;
}

template <traits::FloatingType T>
constexpr RotatorImpl<T> RotatorImpl<T>::operator-(const RotatorImpl& other) const
{
    return RotatorImpl{
        pitch - other.pitch,
        yaw - other.yaw,
        roll - other.roll
    };
}

template <traits::FloatingType T>
RotatorImpl<T>& RotatorImpl<T>::operator-=(const RotatorImpl& other)
{
    pitch -= other.pitch;
    yaw -= other.yaw;
    roll -= other.roll;
    return *this;
}

template <traits::FloatingType T>
template <traits::NumberType Num>
constexpr RotatorImpl<T> RotatorImpl<T>::operator*(Num scale) const
{
    return RotatorImpl{
        pitch * scale,
        yaw * scale,
        roll * scale
    };
}

template <traits::FloatingType T>
template <traits::NumberType Num>
RotatorImpl<T>& RotatorImpl<T>::operator*=(Num scale)
{
    pitch *= scale;
    yaw *= scale;
    roll *= scale;
    return *this;
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> RotatorImpl<T>::GetForwardVector() const
{
    const Radian<T> rad_p{ pitch }; // X축 회전
    const Radian<T> rad_r{ roll };  // Y축 회전
    const Radian<T> rad_y{ yaw };   // Z축 회전

    const T sy = MathUtility::Sin(rad_y), cy = MathUtility::Cos(rad_y);
    const T sp = MathUtility::Sin(rad_p), cp = MathUtility::Cos(rad_p);
    const T sr = MathUtility::Sin(rad_r), cr = MathUtility::Cos(rad_r);

    return Vector3Impl<T>{
        -sy * cr + cy * sp * sr,
        cy * cp,
        sy * sr + cy * sp * cr
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> RotatorImpl<T>::GetRightVector() const
{
    const Radian<T> rad_p{ pitch }; // X축 회전
    const Radian<T> rad_r{ roll };  // Y축 회전
    const Radian<T> rad_y{ yaw };   // Z축 회전

    const T sy = MathUtility::Sin(rad_y), cy = MathUtility::Cos(rad_y);
    const T sp = MathUtility::Sin(rad_p), cp = MathUtility::Cos(rad_p);
    const T sr = MathUtility::Sin(rad_r), cr = MathUtility::Cos(rad_r);

    return Vector3Impl<T>{
        cy * cr + sy * sp * sr,
        sy * cp,
        -cy * sr + sy * sp * cr
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> RotatorImpl<T>::GetUpVector() const
{
    const Radian<T> rad_p{ pitch }; // X축 회전
    const Radian<T> rad_r{ roll };  // Y축 회전
    const Radian<T> rad_y{ yaw };   // Z축 회전

    const T sp = MathUtility::Sin(rad_p), cp = MathUtility::Cos(rad_p);
    const T sr = MathUtility::Sin(rad_r), cr = MathUtility::Cos(rad_r);

    return Vector3Impl<T>{
        cp * sr,
        -sp,
        cp * cr
    };
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T> RotatorImpl<T>::ToQuaternion() const
{
    return QuaternionImpl{ *this };
}

//~ End RotatorImpl
}
