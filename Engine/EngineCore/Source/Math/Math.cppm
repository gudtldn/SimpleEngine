export module SimpleEngine.Math;

export import :MathLiterals;
export import :MathUtility;
import :Matrix;
import :Vector;


export
{
    using namespace se::math::math_literals;

    using Matrix4x4 = MatrixImpl<double, 4, 4, 64>;
    using Matrix4x4f = MatrixImpl<float, 4, 4, 64>;
    using Vector = VectorImpl<double>;
    using Vector3f = VectorImpl<float>;
}
