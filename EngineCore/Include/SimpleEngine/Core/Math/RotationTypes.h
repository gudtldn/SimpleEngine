#pragma once

#include "SimpleEngine/Core/Math/MathFwd.h"
#include "SimpleEngine/Core/Math/MathLiterals.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Utility/Debug.h"


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
    Degree<T> roll;  // Y axis
    Degree<T> yaw;   // Z axis

public:
    constexpr RotatorImpl();
    constexpr RotatorImpl(Degree<T> in_pitch, Degree<T> in_roll, Degree<T> in_yaw);
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
    const Radian<T> half_p{ rotator.pitch * static_cast<T>(0.5) }; // X
    const Radian<T> half_y{ rotator.yaw * static_cast<T>(0.5) };   // Z
    const Radian<T> half_r{ rotator.roll * static_cast<T>(0.5) };  // Y

    // NOLINTBEGIN(*-isolate-declaration)
    const T sp = Sin(half_p), cp = Cos(half_p);
    const T sy = Sin(half_y), cy = Cos(half_y);
    const T sr = Sin(half_r), cr = Cos(half_r);
    // NOLINTEND(*-isolate-declaration)

    // Z-Up, Y-Forward, X-Right 기준의 Z-X-Y 회전 쿼터니언 합성
    x = cy * sp * cr - sy * cp * sr;
    y = cy * cp * sr + sy * sp * cr;
    z = cy * sp * sr + sy * cp * cr;
    w = cy * cp * cr - sy * sp * sr;
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::Identity()
{
    return QuaternionImpl{};
}

template <traits::FloatingType T>
QuaternionImpl<T> QuaternionImpl<T>::FromAxisAngle(const Vector3Impl<T>& axis, Radian<T> angle)
{
    const Radian half_angle = angle * static_cast<T>(0.5);
    const T sin_half_angle = Sin(half_angle);
    const T cos_half_angle = Cos(half_angle);

    SE_ASSERT(axis.IsNormalized());

    return QuaternionImpl{
        axis.x * sin_half_angle,
        axis.y * sin_half_angle,
        axis.z * sin_half_angle,
        cos_half_angle
    };
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
QuaternionImpl<T>& QuaternionImpl<T>::operator*=(const QuaternionImpl& other)
{
    const T tx = w * other.x + x * other.w + y * other.z - z * other.y;
    const T ty = w * other.y - x * other.z + y * other.w + z * other.x;
    const T tz = w * other.z + x * other.y - y * other.x + z * other.w;
    const T tw = w * other.w - x * other.x - y * other.y - z * other.z;

    x = tx;
    y = ty;
    z = tz;
    w = tw;
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

template <traits::FloatingType T>
constexpr void QuaternionImpl<T>::Normalize(T tolerance)
{
    const T square_sum = x * x + y * y + z * z + w * w;
    if (square_sum >= tolerance)
    {
        const T scale = InvSqrt(square_sum);
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
    return Abs(x * x + y * y + z * z + w * w - 1.0f) < tolerance;
}

template <traits::FloatingType T>
constexpr bool QuaternionImpl<T>::IsNearlyEqual(const QuaternionImpl& other, T tolerance) const
{
    return (
        Abs(x - other.x) <= tolerance && Abs(y - other.y) <= tolerance
        && Abs(z - other.z) <= tolerance && Abs(w - other.w) <= tolerance
    ) || (
        Abs(x + other.x) <= tolerance && Abs(y + other.y) <= tolerance
        && Abs(z + other.z) <= tolerance && Abs(w + other.w) <= tolerance
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

//~ End QuaternionImpl

//~ Begin RotatorImpl

template <traits::FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl()
    : pitch(0), roll(0), yaw(0)
{
}

template <traits::FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl(Degree<T> in_pitch, Degree<T> in_roll, Degree<T> in_yaw)
    : pitch(in_pitch), roll(in_roll), yaw(in_yaw)
{
}

template <traits::FloatingType T>
constexpr RotatorImpl<T>::RotatorImpl(const QuaternionImpl<T>& quaternion)
{
    // sin(pitch) 값을 추출해서 짐벌 락 상태인지 체크 (±1에 가까우면 짐벌 락)
    const T sin_p = static_cast<T>(2.0) * (quaternion.y * quaternion.z + quaternion.w * quaternion.x);

    // 짐벌 락 체크 (Pitch가 +/- 90도에 도달한 경우)
    if (Abs(sin_p) >= static_cast<T>(1.0 - KINDA_SMALL_NUMBER))
    {
        // Pitch는 90도 또는 -90도로 고정
        pitch = Degree<T>{ CopySign(static_cast<T>(90.0), sin_p) };

        // 짐벌 락 상태에서는 Yaw와 Roll의 축이 일치하게 됨.
        // Roll을 0으로 강제하고, 모든 회전을 Yaw에 몰아넣어 계산.
        yaw = Degree<T>{ Atan2(
            static_cast<T>(2.0) * (quaternion.x * quaternion.y + quaternion.w * quaternion.z),
            static_cast<T>(1.0) - static_cast<T>(2.0) * (quaternion.y * quaternion.y + quaternion.z * quaternion.z)
        ) };
        roll = Degree<T>{ static_cast<T>(0.0) };
    }
    else
    {
        // Pitch
        pitch = Degree<T>{ Asin(sin_p) };

        // Yaw
        yaw = Degree<T>{ Atan2(
            static_cast<T>(2.0) * (quaternion.w * quaternion.z - quaternion.x * quaternion.y),
            static_cast<T>(1.0) - static_cast<T>(2.0) * (quaternion.x * quaternion.x + quaternion.z * quaternion.z)
        ) };

        // Roll
        roll = Degree<T>{ Atan2(
            static_cast<T>(2.0) * (quaternion.w * quaternion.y - quaternion.x * quaternion.z),
            static_cast<T>(1.0) - static_cast<T>(2.0) * (quaternion.x * quaternion.x + quaternion.y * quaternion.y)
        ) };
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
        roll + other.roll,
        yaw + other.yaw
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
        roll - other.roll,
        yaw - other.yaw
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
        roll * scale,
        yaw * scale
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
    const Radian<T> rad_p{ pitch }; // X축 회전 (상하)
    const Radian<T> rad_y{ yaw };   // Z축 회전 (좌우)

    // NOLINTBEGIN(*-isolate-declaration)
    const T sy = Sin(rad_y), cy = Cos(rad_y);
    const T sp = Sin(rad_p), cp = Cos(rad_p);
    // NOLINTEND(*-isolate-declaration)

    return Vector3Impl<T>{
        -sy * cp, // X (Right): Yaw 회전에 의해 좌우 방향이 결정됨
        cy * cp,  // Y (Forward): Pitch가 커질수록(위/아래를 볼수록) 정면 투영 길이는 줄어듦 (Cos 적용)
        sp        // Z (Up): Yaw에 상관없이 오직 Pitch(상하 고개 각도)에 의해서만 높이가 결정됨
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> RotatorImpl<T>::GetRightVector() const
{
    const Radian<T> rad_p{ pitch }; // X축 회전
    const Radian<T> rad_r{ roll };  // Y축 회전
    const Radian<T> rad_y{ yaw };   // Z축 회전

    // NOLINTBEGIN(*-isolate-declaration)
    const T sy = Sin(rad_y), cy = Cos(rad_y);
    const T sp = Sin(rad_p), cp = Cos(rad_p);
    const T sr = Sin(rad_r), cr = Cos(rad_r);
    // NOLINTEND(*-isolate-declaration)

    // 기본 Right축 (1, 0, 0)에 Roll -> Pitch -> Yaw 순서로 회전 행렬을 곱한 결과
    return Vector3Impl<T>{
        cy * cr - sy * sp * sr,
        sy * cr + cy * sp * sr,
        -cp * sr
    };
}

template <traits::FloatingType T>
constexpr Vector3Impl<T> RotatorImpl<T>::GetUpVector() const
{
    const Radian<T> rad_p{ pitch }; // X축 회전
    const Radian<T> rad_r{ roll };  // Y축 회전
    const Radian<T> rad_y{ yaw };   // Z축 회전

    // NOLINTBEGIN(*-isolate-declaration)
    const T sy = Sin(rad_y), cy = Cos(rad_y);
    const T sp = Sin(rad_p), cp = Cos(rad_p);
    const T sr = Sin(rad_r), cr = Cos(rad_r);
    // NOLINTEND(*-isolate-declaration)

    // 기본 Up축 (0, 0, 1)에 Roll -> Pitch -> Yaw 순서로 회전 행렬을 곱한 결과
    return Vector3Impl<T>{
        cy * sr + sy * sp * cr,
        sy * sr - cy * sp * cr,
        cp * cr
    };
}

template <traits::FloatingType T>
constexpr QuaternionImpl<T> RotatorImpl<T>::ToQuaternion() const
{
    return QuaternionImpl{ *this };
}

//~ End RotatorImpl
} // namespace se::math
