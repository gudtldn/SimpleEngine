export module SimpleEngine.Math;

export import :MathLiterals;
export import :MathUtility;
export import :TransformUtility;
import :Matrix;
import :RotationTypes;
import :Vector2;
import :Vector3;
import :Vector4;


export
{
    using namespace se::math::math_literals;

    // Matrix
    using Matrix4x4 = se::math::Matrix4x4Impl<double>;
    using Matrix4x4f = se::math::Matrix4x4Impl<float>;

    // Vector
    using Vector2 = se::math::Vector2Impl<double>;
    using Vector2f = se::math::Vector2Impl<float>;
    using Vector3 = se::math::Vector3Impl<double>;
    using Vector3f = se::math::Vector3Impl<float>;
    using Vector4 = se::math::Vector4Impl<double>;
    using Vector4f = se::math::Vector4Impl<float>;

    // Quaternion
    using Quaternion = se::math::QuaternionImpl<double>;
    using Quaternionf = se::math::QuaternionImpl<float>;

    // Rotator
    using Rotator = se::math::RotatorImpl<double>;
    using Rotatorf = se::math::RotatorImpl<float>;
}
