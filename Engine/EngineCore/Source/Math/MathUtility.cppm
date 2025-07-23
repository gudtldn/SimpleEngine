export module SimpleEngine.Math:MathUtility;
import :MathLiterals;

import SimpleEngine.TypeTraits;
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
    template <type_traits::OrderableType T>
    [[nodiscard]] static constexpr const T& Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }

    /** 두 값중에 더 큰 값을 반환합니다. */
    template <type_traits::OrderableType T>
    [[nodiscard]] static constexpr const T& Max(const T& a, const T& b)
    {
        return a < b ? b : a;
    }

    /** value를 min과 max의 사이의 값으로 제한합니다. */
    template <type_traits::OrderableType T>
    [[nodiscard]] static constexpr const T& Clamp(const T& value, const T& min_value, const T& max_value)
    {
        return Max(Min(value, max_value), min_value);
    }

    /** value의 절댓값을 구합니다. */
    template <type_traits::OrderableType T>
    [[nodiscard]] static constexpr T Abs(const T value)
    {
        return value < T{ 0 } ? -value : value;
    }

    /** value의 제곱을 구합니다. */
    template <typename T>
    [[nodiscard]] static constexpr T Square(const T value) { return value * value; }

    /** a^b를 반환합니다 */
    [[nodiscard]] static float Pow(float a, float b) { return std::powf(a, b); }
    [[nodiscard]] static double Pow(double a, double b) { return std::pow(a, b); }

    /** value의 제곱근을 구합니다. */
    [[nodiscard]] static float Sqrt(float value) { return std::sqrtf(value); }
    [[nodiscard]] static double Sqrt(double value) { return std::sqrt(value); }

    /** value의 역제곱근을 구합니다. */
    [[nodiscard]] static float InvSqrt(float value) { return 1.0f / std::sqrtf(value); }
    [[nodiscard]] static double InvSqrt(double value) { return 1.0 / std::sqrt(value); }

    /**
     * value가 거의 0에 가까운지 확인합니다.
     * @param value 비교할 숫자
     * @param error_tolerance 거의 0으로 간주되는 값의 허용 최대 차이
     * @return Value가 거의 0에 가까우면 True
     */
    template <typename T>
    [[nodiscard]] static bool IsNearlyZero(T value, T error_tolerance = SMALL_NUMBER)
    {
        return Abs(value) <= error_tolerance;
    }

    /**
     * 두 값이 거의 같은지 확인합니다.
     * @param a 비교할 숫자1
     * @param b 비교할 숫자2
     * @param error_tolerance 거의 0으로 간주되는 값의 허용 최대 차이
     * @return 두 값이 거의 같으면 True
     */
    template <typename T>
    [[nodiscard]] static bool IsNearlyEqual(T a, T b, T error_tolerance = SMALL_NUMBER)
    {
        return Abs(a - b) <= error_tolerance;
    }

    /** Degrees를 Radians으로 변환합니다. */
    template <typename T>
    constexpr Radian<T> DegreesToRadians(Degree<T> degrees)
    {
        return Radian(degrees.value * (static_cast<T>(PI_DOUBLE) / static_cast<T>(180)));
    }

    /** Radians를 Degrees으로 변환합니다. */
    template <typename T>
    constexpr Degree<T> RadiansToDegrees(Radian<T> radians)
    {
        return Degree(radians.value * (static_cast<T>(180) / static_cast<T>(PI_DOUBLE)));
    }
};
}
