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
    static constexpr Matrix4x4Impl<T> MakeModelMatrix(
        const Vector3Impl<T>& translation,
        const RotatorImpl<T>& rotation,
        const Vector3Impl<T>& scale
    )
    {
        return Matrix4x4Impl<T>::MakeFromTranslation(translation)
            * Matrix4x4Impl<T>::MakeFromRotation(rotation)
            * Matrix4x4Impl<T>::MakeFromScale(scale);
    }

    template <FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeModelMatrix(
        const Vector3Impl<T>& translation,
        const QuaternionImpl<T>& quaternion,
        const Vector3Impl<T>& scale
    )
    {
        return Matrix4x4Impl<T>::MakeFromTranslation(translation)
            * Matrix4x4Impl<T>::MakeFromRotation(quaternion)
            * Matrix4x4Impl<T>::MakeFromScale(scale);
    }

    template <FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeViewMatrix(
        const Vector3Impl<T>& position,
        const Vector3Impl<T>& target,
        const Vector3Impl<T>& world_up
    )
    {
        const Vector3Impl<T> f = (target - position).GetNormalized();
        const Vector3Impl<T> r = f.Cross(world_up).GetNormalized();
        const Vector3Impl<T> u = r.Cross(f);

        // +X Right, +Y Forward, +Z Up
        return {
            r.x, r.y, r.z, -r.Dot(position),
            u.x, u.y, u.z, -u.Dot(position),
            -f.x, -f.y, -f.z, f.Dot(position),
            0, 0, 0, 1
        };
    }

    template <FloatingType T>
    static constexpr Matrix4x4Impl<T> MakePerspectiveMatrix(Radian<T> fov, T aspect, T near, T far)
    {
        const T f = 1 / MathUtility::Tan(fov / 2);
        return {
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, far / (near - far), (far * near) / (near - far),
            0, 0, -1, 0
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

        return {
            sx, 0, 0, 0,
            0, sy, 0, 0,
            0, 0, sz, 0,
            tx, ty, tz, 1
        };
    }
};
}
