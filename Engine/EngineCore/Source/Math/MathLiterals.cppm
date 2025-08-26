export module SimpleEngine.Math:MathLiterals;

import SimpleEngine.Traits;
import std;

using namespace se::traits::type_traits;


namespace se::math
{
struct DegreeTag{};
struct RadianTag{};

template <FloatingType NumType, typename UnitTag>
struct AngleType
{
    NumType value;

    constexpr AngleType() = default;

    explicit constexpr AngleType(NumType v)
        : value(v)
    {
    }

    template <FloatingType Other>
        requires std::convertible_to<Other, NumType>
    explicit constexpr AngleType(const AngleType<Other, UnitTag>& other)
        : value(static_cast<NumType>(other.value))
    {
    }

    template <typename OtherUnitTag>
        requires (!std::same_as<UnitTag, OtherUnitTag>)
    explicit constexpr AngleType(const AngleType<NumType, OtherUnitTag>& other)
    {
        if constexpr (std::same_as<UnitTag, DegreeTag>)
        {
            // Radian -> Degree
            value = other.value * (static_cast<NumType>(180) / std::numbers::pi_v<NumType>);
        }
        else
        {
            // Degree -> Radian
            value = other.value * (std::numbers::pi_v<NumType> / static_cast<NumType>(180));
        }
    }

    [[nodiscard]] constexpr AngleType operator+(const AngleType& rhs) const { return AngleType(value + rhs.value); }
    [[nodiscard]] constexpr AngleType operator-(const AngleType& rhs) const { return AngleType(value - rhs.value); }

    AngleType& operator+=(const AngleType& rhs)
    {
        value += rhs.value;
        return *this;
    }

    AngleType& operator-=(const AngleType& rhs)
    {
        value -= rhs.value;
        return *this;
    }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] constexpr AngleType operator+(T scalar) const { return AngleType(value + scalar); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] constexpr AngleType operator-(T scalar) const { return AngleType(value - scalar); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] constexpr AngleType operator*(T scalar) const { return AngleType(value * scalar); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] constexpr AngleType operator/(T scalar) const { return AngleType(value / scalar); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    AngleType& operator+=(T scalar)
    {
        value += scalar;
        return *this;
    }

    template <typename T>
        requires std::convertible_to<T, NumType>
    AngleType& operator-=(T scalar)
    {
        value -= scalar;
        return *this;
    }

    template <typename T>
        requires std::convertible_to<T, NumType>
    AngleType& operator*=(T scalar)
    {
        value *= scalar;
        return *this;
    }

    template <typename T>
        requires std::convertible_to<T, NumType>
    AngleType& operator/=(T scalar)
    {
        value /= scalar;
        return *this;
    }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] friend constexpr AngleType operator+(T scalar, const AngleType& rhs) { return AngleType(scalar + rhs.value); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] friend constexpr AngleType operator-(T scalar, const AngleType& rhs) { return AngleType(scalar - rhs.value); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] friend constexpr AngleType operator*(T scalar, const AngleType& rhs) { return AngleType(scalar * rhs.value); }

    template <typename T>
        requires std::convertible_to<T, NumType>
    [[nodiscard]] friend constexpr AngleType operator/(T scalar, const AngleType& rhs) { return AngleType(scalar / rhs.value); }

    [[nodiscard]] constexpr bool operator==(const AngleType& rhs) const = default;
    [[nodiscard]] constexpr auto operator<=>(const AngleType& rhs) const = default;

    [[nodiscard]] constexpr AngleType operator-() const { return AngleType(-value); }

    [[nodiscard]] constexpr NumType operator*() const { return value; }
    [[nodiscard]] explicit constexpr operator NumType() const { return value; }

    template <typename OtherUnitTag>
        requires (!std::same_as<UnitTag, OtherUnitTag>)
    [[nodiscard]] explicit constexpr operator AngleType<NumType, OtherUnitTag>() const
    {
        return AngleType<NumType, OtherUnitTag>(*this);
    }
};
}

export template <FloatingType NumType>
using Degree = se::math::AngleType<NumType, se::math::DegreeTag>;

export template <FloatingType NumType>
using Radian = se::math::AngleType<NumType, se::math::RadianTag>;


export namespace se::math::math_literals
{
// double literals
constexpr Degree<double> operator"" _deg(long double deg) { return Degree(static_cast<double>(deg)); }
constexpr Degree<double> operator"" _deg(unsigned long long deg) { return Degree(static_cast<double>(deg)); }
constexpr Radian<double> operator"" _rad(long double rad) { return Radian(static_cast<double>(rad)); }
constexpr Radian<double> operator"" _rad(unsigned long long rad) { return Radian(static_cast<double>(rad)); }

// float literals
constexpr Degree<float> operator"" _degf(long double deg) { return Degree(static_cast<float>(deg)); }
constexpr Degree<float> operator"" _degf(unsigned long long deg) { return Degree(static_cast<float>(deg)); }
constexpr Radian<float> operator"" _radf(long double rad) { return Radian(static_cast<float>(rad)); }
constexpr Radian<float> operator"" _radf(unsigned long long rad) { return Radian(static_cast<float>(rad)); }
}
