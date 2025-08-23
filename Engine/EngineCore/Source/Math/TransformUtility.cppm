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
        return Matrix4x4Impl<T>::MakeFromScale(scale)
             * Matrix4x4Impl<T>::MakeFromRotation(rotation)
             * Matrix4x4Impl<T>::MakeFromTranslation(translation);
    }

    template <FloatingType T>
    static constexpr Matrix4x4Impl<T> MakeModelMatrix(
        const Vector3Impl<T>& translation,
        const QuaternionImpl<T>& quaternion,
        const Vector3Impl<T>& scale
    )
    {
        return Matrix4x4Impl<T>::MakeFromScale(scale)
             * Matrix4x4Impl<T>::MakeFromRotation(quaternion)
             * Matrix4x4Impl<T>::MakeFromTranslation(translation);
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
