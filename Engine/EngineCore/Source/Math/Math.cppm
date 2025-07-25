export module SimpleEngine.Math;

export import :MathLiterals;
export import :MathUtility;
import :Matrix;
import :Vector2;
import :Vector3;


export
{
    using namespace se::math::math_literals;

    // Matrix
    using Matrix4x4 = Matrix4x4Impl<double, 64>;
    using Matrix4x4f = Matrix4x4Impl<float, 64>;

    // Vector
    using Vector2 = Vector2Impl<double>;
    using Vector2f = Vector2Impl<float>;
    using Vector3 = Vector3Impl<double>;
    using Vector3f = Vector3Impl<float>;
}
