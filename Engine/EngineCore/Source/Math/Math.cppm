export module SimpleEngine.Math;

export import :MathLiterals;
export import :MathUtility;
import :Matrix;
import :Vector;


export
{
    using namespace se::math::math_literals;

    using Matrix4x4 = Matrix4x4Impl<double, 64>;
    using Matrix4x4f = Matrix4x4Impl<float, 64>;
    using Vector3 = VectorImpl<double>;
    using Vector3f = VectorImpl<float>;
}
