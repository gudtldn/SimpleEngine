#pragma once
#include <cmath>
#include <numbers>

#include "SimpleEngine/Core/Math/MathLiterals.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se::math
{
constexpr float SMALL_NUMBER = 1.e-8f;
constexpr float KINDA_SMALL_NUMBER = 1.e-4f;

template <traits::FloatingType T>
constexpr T PI_V = std::numbers::pi_v<T>;

constexpr float PI = PI_V<float>;
constexpr float INV_PI = std::numbers::inv_pi_v<float>;
constexpr float HALF_PI = PI * 0.5f;
constexpr float TWO_PI = PI * 2.0f;
constexpr float PI_SQUARED = PI * PI;

constexpr double PI_DOUBLE = PI_V<double>;


// TODO: C++26 컴파일러 나오면 기본 std 함수로 대체
namespace detail
{
template <typename T>
constexpr T AbsImpl(const T value)
{
    return value < T(0) ? -value : value;
}

template <typename T>
constexpr bool IsNaN(T x)
{
    // ReSharper disable once CppIdenticalOperandsInBinaryExpression
    return x != x; // NOLINT(*-redundant-expression)
}

template <typename T>
constexpr bool IsInfinite(T x)
{
    return x == std::numeric_limits<T>::infinity()
        || x == -std::numeric_limits<T>::infinity();
}

template <typename T>
constexpr bool IsFinite(T x)
{
    return !IsNaN(x) && !IsInfinite(x);
}

template <typename T>
constexpr T Fmod(T x, T y)
{
    if (AbsImpl(y) <= std::numeric_limits<T>::epsilon()) { return T(0); }

    const int64 div = static_cast<int64>(x / y);
    return x - (static_cast<T>(div) * y);
}

template <typename T>
constexpr T Sqrt(T x)
{
    if (x < T(0)) { return std::numeric_limits<T>::quiet_NaN(); }
    if (x == T(0)) { return T(0); }

    T curr = x;
    T prev = T(0);
    for (usize i = 0; i < 20; ++i)
    {
        prev = curr;
        curr = T(0.5) * (curr + (x / curr));
        if (AbsImpl(curr - prev) <= std::numeric_limits<T>::epsilon())
        {
            break;
        }
    }
    return curr;
}

// e^x (Maclaurin Series)
template <typename T>
constexpr T Exp(T x)
{
    T sum = T(1);
    T term = T(1);
    for (usize i = 1; i < 30; ++i)
    {
        term *= x / static_cast<T>(i);
        sum += term;
        if (AbsImpl(term) < std::numeric_limits<T>::epsilon())
        {
            break;
        }
    }
    return sum;
}

// ln(x) (Newton-Raphson)
template <typename T>
constexpr T Ln(T x)
{
    if (x <= T(0)) { return -std::numeric_limits<T>::infinity(); }

    T y = (x > T(1)) ? T(1) : T(0); // initial guess
    for (usize i = 0; i < 20; ++i)
    {
        T ey = Exp(y);
        y -= (ey - x) / ey;
    }
    return y;
}

template <typename T>
constexpr T Pow(T base, T exp)
{
    if (base == T(0)) { return T(0); };
    if (exp == T(0))  { return T(1); };
    if (exp == T(1))  { return base; };

    // x^y = e^(y * ln(x))
    if (base < T(0))
    {
        // 음수의 정수승 처리 (e.g. -2^3)
        if (Fmod(exp, T(1)) == T(0))
        {
            T res = Exp(exp * Ln(-base));
            return (static_cast<int64>(exp) % 2 == 0) ? res : -res;
        }
        return std::numeric_limits<T>::quiet_NaN();
    }
    return Exp(exp * Ln(base));
}

template <typename T>
[[nodiscard]] constexpr T CopySign(T num, T sign)
{
    return (sign >= T(0)) ? AbsImpl(num) : -AbsImpl(num);
}

template <typename T>
constexpr T FloorImpl(T x)
{
    const int64 i = static_cast<int64>(x);
    return (x < T(0) && x != static_cast<T>(i)) ? static_cast<T>(i - 1) : static_cast<T>(i);
}

template <typename T>
constexpr T CeilImpl(T x)
{
    const int64 i = static_cast<int64>(x);
    return (x > T(0) && x != static_cast<T>(i)) ? static_cast<T>(i + 1) : static_cast<T>(i);
}

/** 반올림 (Round) - .5일 때 0에서 먼 쪽으로 */
template <typename T>
constexpr T RoundImpl(T x)
{
    return (x >= T(0)) ? FloorImpl(x + T(0.5)) : CeilImpl(x - T(0.5));
}
}  // namespace detail


/** 두 값중에 더 작은 값을 반환합니다. */
template <traits::Orderable T>
[[nodiscard]] static constexpr const T& Min(const T& a, const T& b)
{
    return a < b ? a : b; // NOLINT(*-return-const-ref-from-parameter)
}

/** 두 값중에 더 큰 값을 반환합니다. */
template <traits::Orderable T>
[[nodiscard]] static constexpr const T& Max(const T& a, const T& b)
{
    return a < b ? b : a; // NOLINT(*-return-const-ref-from-parameter)
}

/** value를 min과 max의 사이의 값으로 제한합니다. */
template <traits::Orderable T>
[[nodiscard]] static constexpr const T& Clamp(const T& value, const T& min_value, const T& max_value)
{
    return Max(Min(value, max_value), min_value);
}

/** value의 절댓값을 구합니다. */
template <traits::Orderable T>
[[nodiscard]] static constexpr T Abs(const T value)
{
    return value < T{ 0 } ? -value : value;
}

/** 0으로 나누기를 방지하는 안전한 나눗셈 연산을 수행합니다. */
template <traits::NumberType T>
[[nodiscard]] static constexpr T SafeDivide(const T a, const T b, const T tolerance = KINDA_SMALL_NUMBER)
{
    if (Abs(b) <= tolerance)
    {
        return T{ 0 };
    }
    return a / b;
}

/** 주어진 값이 유한수인지 확인합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr bool IsFinite(T value)
{
    if consteval { return detail::IsFinite(value); }
    return std::isfinite(value);
}

/** value의 제곱을 구합니다. */
template <typename T>
[[nodiscard]] static constexpr T Square(const T value) { return value * value; }

/** a^b를 반환합니다 */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Pow(T a, T b)
{
    if consteval { return detail::Pow(a, b); }
    return std::pow(a, b);
}

/** value의 제곱근을 구합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Sqrt(T value)
{
    if consteval { return detail::Sqrt(value); }
    return std::sqrt(value);
}

/** value의 역제곱근을 구합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T InvSqrt(T value) { return static_cast<T>(1) / Sqrt(value); }

/** e^value를 반환합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Exp(T value)
{
    if consteval { return detail::Exp(value); }
    return std::exp(value);
}

/** 자연로그 ln(value)를 반환합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Ln(T value)
{
    if consteval { return detail::Ln(value); }
    return std::log(value);
}

template <traits::FloatingType T>
[[nodiscard]] static constexpr T Fmod(T value, T mod)
{
    if consteval { return detail::Fmod(value, mod); }
    return std::fmod(value, mod);
}

template <traits::FloatingType T>
[[nodiscard]] static constexpr T Cos(Radian<T> rad_value) { return std::cos(*rad_value); }

template <traits::FloatingType T>
[[nodiscard]] static constexpr T Sin(Radian<T> rad_value) { return std::sin(*rad_value); }

template <traits::FloatingType T>
[[nodiscard]] static constexpr T Tan(Radian<T> rad_value) { return std::tan(*rad_value); }

template <traits::FloatingType T>
[[nodiscard]] static constexpr Radian<T> Acos(T value) { return Radian<T>(std::acos(value)); }

template <traits::FloatingType T>
[[nodiscard]] static constexpr Radian<T> Asin(T value) { return Radian<T>(std::asin(value)); }

template <traits::FloatingType T>
[[nodiscard]] static constexpr Radian<T> Atan(T value) { return Radian<T>(std::atan(value)); }

template <traits::FloatingType T>
[[nodiscard]] static constexpr Radian<T> Atan2(T y, T x) { return Radian<T>(std::atan2(y, x)); }

/** 주어진 num에 sign의 부호를 적용합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T CopySign(T num, T sign)
{
    if consteval { return detail::CopySign(num, sign); }
    return std::copysign(num, sign);
}

/** value를 내림합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Floor(T value)
{
    if consteval { return detail::FloorImpl(value); }
    return std::floor(value);
}

/** value를 올립니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Ceil(T value)
{
    if consteval { return detail::CeilImpl(value); }
    return std::ceil(value);
}

/** 가장 가까운 정수로 반올림합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr T Round(T value)
{
    if consteval { return detail::RoundImpl(value); }
    return std::round(value);
}

/** 반올림 후 원하는 정수 타입으로 캐스팅하여 반환합니다. */
template <typename IntType = int32, traits::FloatingType FloatType>
[[nodiscard]] static constexpr IntType RoundToInt(FloatType value)
{
    return static_cast<IntType>(Round(value));
}

/** 내림 후 원하는 정수 타입으로 캐스팅하여 반환합니다. */
template <typename IntType = int32, traits::FloatingType FloatType>
[[nodiscard]] static constexpr IntType FloorToInt(FloatType value)
{
    return static_cast<IntType>(Floor(value));
}

/** 올림 후 원하는 정수 타입으로 캐스팅하여 반환합니다. */
template <typename IntType = int32, traits::FloatingType FloatType>
[[nodiscard]] static constexpr IntType CeilToInt(FloatType value)
{
    return static_cast<IntType>(Ceil(value));
}

/** value가 거의 0에 가까운지 확인합니다. */
template <typename T>
[[nodiscard]] static constexpr bool IsNearlyZero(T value, T tolerance = KINDA_SMALL_NUMBER)
{
    return Abs(value) <= tolerance;
}

/** 두 값이 거의 같은지 확인합니다. */
template <typename T>
[[nodiscard]] static constexpr bool IsNearlyEqual(T a, T b, T tolerance = KINDA_SMALL_NUMBER)
{
    return Abs(a - b) <= tolerance;
}

/** Degrees를 Radians으로 변환합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr Radian<T> DegToRad(T degrees)
{
    return Radian{ degrees * (static_cast<T>(PI_V<T>) / static_cast<T>(180)) };
}

/** Radians를 Degrees으로 변환합니다. */
template <traits::FloatingType T>
[[nodiscard]] static constexpr Degree<T> RadToDeg(T radians)
{
    return Degree{ radians * (static_cast<T>(180) / static_cast<T>(PI_V<T>)) };
}
}  // namespace se::math
