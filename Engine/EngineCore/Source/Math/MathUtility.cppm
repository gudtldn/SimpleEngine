export module SimpleEngine.Math:MathUtility;
import :MathLiterals;

import SimpleEngine.Traits;
import SimpleEngine.Types;
import std;


namespace se::math
{
using namespace math_literals;

constexpr float SMALL_NUMBER = 1.e-8f;
constexpr float KINDA_SMALL_NUMBER = 1.e-4f;

constexpr float PI = 3.1415926535897932f;
constexpr float INV_PI = 0.31830988618f;
constexpr float HALF_PI = 1.57079632679f;
constexpr float TWO_PI = 6.28318530717f;
constexpr float PI_SQUARED = 9.86960440108f;

constexpr double PI_DOUBLE = 3.141592653589793238462643383279502884197169399;


export struct MathUtils
{
    /** 두 값중에 더 작은 값을 반환합니다. */
    template <OrderableType T>
    [[nodiscard]] static constexpr const T& Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }

    /** 두 값중에 더 큰 값을 반환합니다. */
    template <OrderableType T>
    [[nodiscard]] static constexpr const T& Max(const T& a, const T& b)
    {
        return a < b ? b : a;
    }

    /** value를 min과 max의 사이의 값으로 제한합니다. */
    template <OrderableType T>
    [[nodiscard]] static constexpr const T& Clamp(const T& value, const T& min_value, const T& max_value)
    {
        return Max(Min(value, max_value), min_value);
    }

    /** value의 절댓값을 구합니다. */
    template <OrderableType T>
    [[nodiscard]] static constexpr T Abs(const T value)
    {
        return value < T{ 0 } ? -value : value;
    }

    /** 0으로 나누기를 방지하는 안전한 나눗셈 연산을 수행합니다. */
    template <NumberType T>
    [[nodiscard]] static constexpr T SafeDivide(const T a, const T b, const T esp = KINDA_SMALL_NUMBER)
    {
        if (Abs(b) <= esp)
        {
            return T{ 0 };
        }
        return a / b;
    }

    /** 주어진 값이 유한수인지 확인합니다. */
    template <FloatingType T>
    [[nodiscard]] static constexpr T IsFinite(T value)
    {
        return std::isfinite(value);
    }

    /** value의 제곱을 구합니다. */
    template <typename T>
    [[nodiscard]] static constexpr T Square(const T value) { return value * value; }

    /** a^b를 반환합니다 */
    template <FloatingType T>
    [[nodiscard]] static constexpr T Pow(T a, T b) { return std::pow(a, b); }

    /** value의 제곱근을 구합니다. */
    template <FloatingType T>
    [[nodiscard]] static constexpr T Sqrt(T value) { return std::sqrt(value); }

    /** value의 역제곱근을 구합니다. */
    template <FloatingType T>
    [[nodiscard]] static constexpr T InvSqrt(T value) { return static_cast<T>(1) / std::sqrt(value); }

    template <FloatingType T>
    [[nodiscard]] static constexpr T Fmod(T value, T mod) { return std::fmod(value, mod); }

    template <FloatingType T>
    [[nodiscard]] static constexpr T Cos(Radian<T> rad_value) { return std::cos(*rad_value); }

    template <FloatingType T>
    [[nodiscard]] static constexpr T Sin(Radian<T> rad_value) { return std::sin(*rad_value); }

    template <FloatingType T>
    [[nodiscard]] static constexpr T Tan(Radian<T> rad_value) { return std::tan(*rad_value); }

    template <FloatingType T>
    [[nodiscard]] static constexpr Radian<T> Acos(T value) { return Radian<T>(std::acos(value)); }

    template <FloatingType T>
    [[nodiscard]] static constexpr Radian<T> Asin(T value) { return Radian<T>(std::asin(value)); }

    template <FloatingType T>
    [[nodiscard]] static constexpr Radian<T> Atan(T value) { return Radian<T>(std::atan(value)); }

    template <FloatingType T>
    [[nodiscard]] static constexpr Radian<T> Atan2(T y, T x) { return Radian<T>(std::atan2(y, x)); }

    /** 주어진 num에 sign의 부호를 적용합니다. */
    template <FloatingType T>
    [[nodiscard]] static constexpr T CopySign(T num, T sign) { return std::copysign(num, sign); }

    /** value가 거의 0에 가까운지 확인합니다. */
    template <typename T>
    [[nodiscard]] static constexpr bool IsNearlyZero(T value, T error_tolerance = SMALL_NUMBER)
    {
        return Abs(value) <= error_tolerance;
    }

    /** 두 값이 거의 같은지 확인합니다. */
    template <typename T>
    [[nodiscard]] static constexpr bool IsNearlyEqual(T a, T b, T error_tolerance = SMALL_NUMBER)
    {
        return Abs(a - b) <= error_tolerance;
    }

    /** Degrees를 Radians으로 변환합니다. */
    template <FloatingType T>
    [[nodiscard]] static constexpr Radian<T> DegreesToRadians(Degree<T> degrees)
    {
        return Radian(degrees.value * (static_cast<T>(PI_DOUBLE) / static_cast<T>(180)));
    }

    /** Radians를 Degrees으로 변환합니다. */
    template <FloatingType T>
    [[nodiscard]] static constexpr Degree<T> RadiansToDegrees(Radian<T> radians)
    {
        return Degree(radians.value * (static_cast<T>(180) / static_cast<T>(PI_DOUBLE)));
    }
};
}
