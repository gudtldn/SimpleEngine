export module SimpleEngine.Math:TransformUtility;
import :MathUtility;
import :Matrix;
import :RotationTypes;
import :Vector3;

import SimpleEngine.Traits;
import SimpleEngine.Types;
import std;


namespace se::math
{
export struct TransformUtility
{
    template <FloatingType T>
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

    template <FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeFromRotation(const RotatorImpl<T>& rotation)
    {
        // https://en.wikipedia.org/wiki/Rotation_matrix
        // 위키피디아에 나온 공식은 RH, col-major
        // 여기서는 RH, row-major이기 때문에 전치해서 사용

        const Radian<T> rad_p{ rotation.pitch };
        const Radian<T> rad_y{ rotation.yaw };
        const Radian<T> rad_r{ rotation.roll };

        const T sin_p = MathUtility::Sin(rad_p), cos_p = MathUtility::Cos(rad_p);
        const T sin_y = MathUtility::Sin(rad_y), cos_y = MathUtility::Cos(rad_y);
        const T sin_r = MathUtility::Sin(rad_r), cos_r = MathUtility::Cos(rad_r);

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

    template <FloatingType T>
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

    template <FloatingType T>
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

    template <FloatingType T>
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

    template <FloatingType T>
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

    template <FloatingType T>
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

    template <FloatingType T>
    static constexpr Matrix4x4Impl<T> MakePerspectiveMatrix(Radian<T> fov_y, T aspect, T near, T far)
    {
        const T f = 1 / MathUtility::Tan(fov_y * static_cast<T>(0.5));
        return Matrix4x4Impl<T>{
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, far / (near - far), -1,
            0, 0, (near * far) / (near - far), 0
        };
    }

    template <FloatingType T>
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

    template <FloatingType T>
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
};
}
