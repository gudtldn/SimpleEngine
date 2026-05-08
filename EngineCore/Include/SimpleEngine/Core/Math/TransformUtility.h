#pragma once

#include "SimpleEngine/Core/Math/MathFwd.h"
#include "SimpleEngine/Core/Math/MathUtility.h"


namespace se::math
{
struct TransformUtility
{
    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeFromTranslation(const Vector3Impl<T>& translation)
    {
        T x = translation.x;
        T y = translation.y;
        T z = translation.z;

        return Matrix4x4Impl<T>{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            x, y, z, 1
        };
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeFromRotation(const RotatorImpl<T>& rotation)
    {
        // https://en.wikipedia.org/wiki/Rotation_matrix
        // 위키피디아에 나온 공식은 RH, col-major
        // 여기서는 RH, row-major이기 때문에 전치해서 사용

        const Radian<T> rad_p{ rotation.pitch };
        const Radian<T> rad_y{ rotation.yaw };
        const Radian<T> rad_r{ rotation.roll };

        const T sin_p = Sin(rad_p), cos_p = Cos(rad_p);
        const T sin_y = Sin(rad_y), cos_y = Cos(rad_y);
        const T sin_r = Sin(rad_r), cos_r = Cos(rad_r);

        // Rz(yaw)
        Matrix4x4Impl<T> rz{
            cos_y, sin_y, 0, 0,
            -sin_y, cos_y, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };

        // Rx(pitch)
        Matrix4x4Impl<T> rx{
            1, 0, 0, 0,
            0, cos_p, sin_p, 0,
            0, -sin_p, cos_p, 0,
            0, 0, 0, 1
        };

        // Ry(roll)
        Matrix4x4Impl<T> ry{
            cos_r, 0, -sin_r, 0,
            0, 1, 0, 0,
            sin_r, 0, cos_r, 0,
            0, 0, 0, 1
        };

        // Rz(yaw) * Rx(pitch) * Ry(roll)
        return rz * rx * ry;
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeFromRotation(const QuaternionImpl<T>& quaternion)
    {
        const T x = quaternion.x;
        const T y = quaternion.y;
        const T z = quaternion.z;
        const T w = quaternion.w;

        const T xx = x * x;
        const T yy = y * y;
        const T zz = z * z;
        const T xy = x * y;
        const T xz = x * z;
        const T yz = y * z;
        const T wx = w * x;
        const T wy = w * y;
        const T wz = w * z;

        // Row-major 3x3 rotation block for row-vector (p' = p M)
        return Matrix4x4Impl<T>{
            1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0,
            2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx), 0,
            2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy), 0,
            0, 0, 0, 1
        };
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeFromScale(const Vector3Impl<T>& scale)
    {
        T x = scale.x;
        T y = scale.y;
        T z = scale.z;

        return Matrix4x4Impl<T>{
            x, 0, 0, 0,
            0, y, 0, 0,
            0, 0, z, 0,
            0, 0, 0, 1
        };
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeModelMatrix(
        const Vector3Impl<T>& translation,
        const RotatorImpl<T>& rotation,
        const Vector3Impl<T>& scale
    )
    {
        return MakeFromScale(scale)
            * MakeFromRotation(rotation)
            * MakeFromTranslation(translation);
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeModelMatrix(
        const Vector3Impl<T>& translation,
        const QuaternionImpl<T>& quaternion,
        const Vector3Impl<T>& scale
    )
    {
        return MakeFromScale(scale)
            * MakeFromRotation(quaternion)
            * MakeFromTranslation(translation);
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeViewMatrix(
        const Vector3Impl<T>& position,
        const Vector3Impl<T>& target,
        const Vector3Impl<T>& world_up
    )
    {
        const Vector3Impl<T> f = (position - target).GetNormalized();
        const Vector3Impl<T> r = world_up.Cross(f).GetNormalized();
        const Vector3Impl<T> u = f.Cross(r);

        return Matrix4x4Impl<T>{
            r.x, u.x, f.x, 0,
            r.y, u.y, f.y, 0,
            r.z, u.z, f.z, 0,
            -r.Dot(position), -u.Dot(position), -f.Dot(position), 1
        };
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakePerspectiveMatrix(Radian<T> fov_y, T aspect, T near, T far)
    {
        const T f = 1 / Tan(fov_y * static_cast<T>(0.5));
        return Matrix4x4Impl<T>{
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, far / (near - far), -1,
            0, 0, (near * far) / (near - far), 0
        };
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeOrthographicMatrix(T left, T right, T bottom, T top, T near, T far)
    {
        const T sx = 2 / (right - left);
        const T sy = 2 / (top - bottom);
        const T sz = -1 / (far - near);

        const T tx = -(right + left) / (right - left);
        const T ty = -(top + bottom) / (top - bottom);
        const T tz = -near / (far - near);

        return Matrix4x4Impl<T>{
            sx, 0, 0, 0,
            0, sy, 0, 0,
            0, 0, sz, 0,
            tx, ty, tz, 1
        };
    }

    template <traits::FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeOrthographicMatrix(T width, T height, T near, T far)
    {
        const T half_width = width * static_cast<T>(0.5);
        const T half_height = height * static_cast<T>(0.5);

        return MakeOrthographicMatrix(
            -half_width, half_width,
            -half_height, half_height,
            near,
            far
        );
    }

    // -- Model Matrix Decomposition --
    // MakeModelMatrix(translation, rotation, scale) = S * R * T 의 역연산
    //
    // Row-major 결과 행렬 레이아웃:
    //   Row 0: sx*r00, sx*r01, sx*r02, 0
    //   Row 1: sy*r10, sy*r11, sy*r12, 0
    //   Row 2: sz*r20, sz*r21, sz*r22, 0
    //   Row 3: tx,     ty,     tz,     1

    template <traits::FloatingType T>
    static constexpr Vector3Impl<T> DecomposeTranslation(const Matrix4x4Impl<T>& m)
    {
        return { m[3, 0], m[3, 1], m[3, 2] };
    }

    template <traits::FloatingType T>
    static constexpr Vector3Impl<T> DecomposeScale(const Matrix4x4Impl<T>& m)
    {
        T sx = Vector3Impl<T>(m[0, 0], m[0, 1], m[0, 2]).Length();
        const T sy = Vector3Impl<T>(m[1, 0], m[1, 1], m[1, 2]).Length();
        const T sz = Vector3Impl<T>(m[2, 0], m[2, 1], m[2, 2]).Length();

        // 음수 스케일(미러링) 감지: 상위 3x3 행렬식이 음수이면 X축 부호 반전
        const Vector3Impl<T> row0{ m[0, 0], m[0, 1], m[0, 2] };
        const Vector3Impl<T> row1{ m[1, 0], m[1, 1], m[1, 2] };
        const Vector3Impl<T> row2{ m[2, 0], m[2, 1], m[2, 2] };
        if (row0.Dot(row1.Cross(row2)) < static_cast<T>(0))
        {
            sx = -sx;
        }

        return { sx, sy, sz };
    }

    template <traits::FloatingType T>
    static constexpr QuaternionImpl<T> DecomposeRotation(const Matrix4x4Impl<T>& m)
    {
        // Scale 제거: 각 행을 정규화하여 순수 회전 행렬 추출
        const Vector3Impl<T> scale = DecomposeScale(m);
        const T inv_sx = (scale.x > KINDA_SMALL_NUMBER) ? static_cast<T>(1) / scale.x : static_cast<T>(0);
        const T inv_sy = (scale.y > KINDA_SMALL_NUMBER) ? static_cast<T>(1) / scale.y : static_cast<T>(0);
        const T inv_sz = (scale.z > KINDA_SMALL_NUMBER) ? static_cast<T>(1) / scale.z : static_cast<T>(0);

        // 정규화된 회전 행렬 (rm = row-major, row-vector convention)
        const T rm00 = m[0, 0] * inv_sx, rm01 = m[0, 1] * inv_sx, rm02 = m[0, 2] * inv_sx;
        const T rm10 = m[1, 0] * inv_sy, rm11 = m[1, 1] * inv_sy, rm12 = m[1, 2] * inv_sy;
        const T rm20 = m[2, 0] * inv_sz, rm21 = m[2, 1] * inv_sz, rm22 = m[2, 2] * inv_sz;

        // Shepperd's method (row-major, row-vector convention: R_col = R_row^T)
        // x = (rm12 - rm21) / s,  y = (rm20 - rm02) / s,  z = (rm01 - rm10) / s
        QuaternionImpl<T> q;
        const T trace = rm00 + rm11 + rm22;

        if (trace > static_cast<T>(0))
        {
            const T s = Sqrt(trace + static_cast<T>(1)) * static_cast<T>(2);
            q.w = s / static_cast<T>(4);
            q.x = (rm12 - rm21) / s;
            q.y = (rm20 - rm02) / s;
            q.z = (rm01 - rm10) / s;
        }
        else if (rm00 > rm11 && rm00 > rm22)
        {
            const T s = Sqrt(static_cast<T>(1) + rm00 - rm11 - rm22) * static_cast<T>(2);
            q.w = (rm12 - rm21) / s;
            q.x = s / static_cast<T>(4);
            q.y = (rm01 + rm10) / s;
            q.z = (rm02 + rm20) / s;
        }
        else if (rm11 > rm22)
        {
            const T s = Sqrt(static_cast<T>(1) + rm11 - rm00 - rm22) * static_cast<T>(2);
            q.w = (rm20 - rm02) / s;
            q.x = (rm01 + rm10) / s;
            q.y = s / static_cast<T>(4);
            q.z = (rm12 + rm21) / s;
        }
        else
        {
            const T s = Sqrt(static_cast<T>(1) + rm22 - rm00 - rm11) * static_cast<T>(2);
            q.w = (rm01 - rm10) / s;
            q.x = (rm02 + rm20) / s;
            q.y = (rm12 + rm21) / s;
            q.z = s / static_cast<T>(4);
        }

        return q;
    }
};
}
