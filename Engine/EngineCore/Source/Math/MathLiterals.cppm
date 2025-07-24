export module SimpleEngine.Math:MathLiterals;

import SimpleEngine.TypeTraits;
import std;

using namespace se::type_traits;


struct DegreeTag{};
struct RadianTag{};

template <FloatingType NumType, typename UnitTag>
struct AngleType
{
    NumType value;

    explicit constexpr AngleType(NumType v)
        : value(v)
    {
    }

    template <FloatingType Other>
        requires std::is_convertible_v<Other, NumType>
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

    constexpr AngleType& operator+=(const AngleType& rhs) { value += rhs.value; return *this; }
    constexpr AngleType& operator-=(const AngleType& rhs) { value -= rhs.value; return *this; }

    [[nodiscard]] constexpr AngleType operator+(NumType scalar) const { return AngleType(value + scalar); }
    [[nodiscard]] constexpr AngleType operator-(NumType scalar) const { return AngleType(value - scalar); }
    [[nodiscard]] constexpr AngleType operator*(NumType scalar) const { return AngleType(value * scalar); }

    constexpr AngleType& operator+=(NumType scalar) { value += scalar; return *this; }
    constexpr AngleType& operator-=(NumType scalar) { value -= scalar; return *this; }
    constexpr AngleType& operator*=(NumType scalar) { value *= scalar; return *this; }

    [[nodiscard]] constexpr AngleType operator-() const { return AngleType(-value); }

    [[nodiscard]] friend constexpr AngleType operator+(NumType scalar, const AngleType& deg) { return AngleType(scalar + deg.value); }
    [[nodiscard]] friend constexpr AngleType operator-(NumType scalar, const AngleType& deg) { return AngleType(scalar - deg.value); }
    [[nodiscard]] friend constexpr AngleType operator*(NumType scalar, const AngleType& deg) { return AngleType(scalar * deg.value); }

    [[nodiscard]] constexpr bool operator==(const AngleType& rhs) const = default;
    [[nodiscard]] constexpr auto operator<=>(const AngleType& rhs) const = default;

    [[nodiscard]] constexpr NumType operator*() const { return value; }
    [[nodiscard]] explicit constexpr operator NumType() const { return value; }

    template <typename OtherUnitTag>
        requires (!std::same_as<UnitTag, OtherUnitTag>)
    [[nodiscard]] explicit constexpr operator AngleType<NumType, OtherUnitTag>() const
    {
        return AngleType<NumType, OtherUnitTag>(*this);
    }
};

export template <FloatingType NumType>
using Degree = AngleType<NumType, DegreeTag>;

export template <FloatingType NumType>
using Radian = AngleType<NumType, RadianTag>;


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
