export module SimpleEngine.Math:MathLiterals;

import SimpleEngine.TypeTraits;
import std;

using namespace se::type_traits;


// TODO: 추후 Tag 패턴으로 변경
export template <NumberType>
struct Radian;

export template <NumberType NumType>
struct Degree
{
    NumType value;

    explicit constexpr Degree(NumType v)
        : value(v)
    {
    }

    template <typename Other>
        requires std::is_convertible_v<Other, NumType>
    explicit constexpr Degree(const Degree<Other>& other)
        : value(static_cast<NumType>(other.value))
    {
    }

    explicit constexpr Degree(const Radian<NumType>& rad)
        : value(static_cast<NumType>(rad) * (static_cast<NumType>(180) / static_cast<NumType>(std::numbers::pi)))
    {
    }

    [[nodiscard]] constexpr Degree operator+(const Degree& rhs) const { return Degree(value + rhs.value); }
    [[nodiscard]] constexpr Degree operator-(const Degree& rhs) const { return Degree(value - rhs.value); }
    constexpr Degree& operator+=(const Degree& rhs) { value += rhs.value; return *this; }
    constexpr Degree& operator-=(const Degree& rhs) { value -= rhs.value; return *this; }

    [[nodiscard]] constexpr Degree operator+(NumType scalar) const { return Degree(value + scalar); }
    [[nodiscard]] constexpr Degree operator-(NumType scalar) const { return Degree(value - scalar); }
    [[nodiscard]] constexpr Degree operator*(NumType scalar) const { return Degree(value * scalar); }
    [[nodiscard]] constexpr Degree operator/(NumType scalar) const { return Degree(value / scalar); }

    constexpr Degree& operator+=(NumType scalar) { value += scalar; return *this; }
    constexpr Degree& operator-=(NumType scalar) { value -= scalar; return *this; }
    constexpr Degree& operator*=(NumType scalar) { value *= scalar; return *this; }
    constexpr Degree& operator/=(NumType scalar) { value /= scalar; return *this; }

    [[nodiscard]] constexpr Degree operator-() const { return Degree(-value); }

    [[nodiscard]] friend constexpr Degree operator+(NumType scalar, const Degree& deg) { return Degree(scalar + deg.value); }
    [[nodiscard]] friend constexpr Degree operator-(NumType scalar, const Degree& deg) { return Degree(scalar - deg.value); }
    [[nodiscard]] friend constexpr Degree operator*(NumType scalar, const Degree& deg) { return Degree(scalar * deg.value); }
    [[nodiscard]] friend constexpr Degree operator/(NumType scalar, const Degree& deg) { return Degree(scalar / deg.value); }

    [[nodiscard]] constexpr bool operator==(const Degree& rhs) const = default;
    [[nodiscard]] constexpr auto operator<=>(const Degree& rhs) const = default;

    [[nodiscard]] constexpr NumType operator*() const { return value; }
    [[nodiscard]] explicit constexpr operator NumType() const { return value; }
    [[nodiscard]] explicit constexpr operator Radian<NumType>() const { return Radian<NumType>(*this); }
};

export template <NumberType NumType>
struct Radian
{
    NumType value;

    explicit constexpr Radian(NumType v)
        : value(v)
    {
    }

    template <typename Other>
        requires std::is_convertible_v<Other, NumType>
    explicit constexpr Radian(const Radian<Other>& other)
        : value(static_cast<NumType>(other.value))
    {
    }

    explicit constexpr Radian(const Degree<NumType>& deg)
        : value(static_cast<NumType>(deg) * (static_cast<NumType>(std::numbers::pi) / static_cast<NumType>(180)))
    {
    }

    [[nodiscard]] constexpr Radian operator+(const Radian& rhs) const { return Radian(value + rhs.value); }
    [[nodiscard]] constexpr Radian operator-(const Radian& rhs) const { return Radian(value - rhs.value); }
    constexpr Radian& operator+=(const Radian& rhs) { value += rhs.value; return *this; }
    constexpr Radian& operator-=(const Radian& rhs) { value -= rhs.value; return *this; }

    [[nodiscard]] constexpr Radian operator+(NumType scalar) const { return Radian(value + scalar); }
    [[nodiscard]] constexpr Radian operator-(NumType scalar) const { return Radian(value - scalar); }
    [[nodiscard]] constexpr Radian operator*(NumType scalar) const { return Radian(value * scalar); }
    [[nodiscard]] constexpr Radian operator/(NumType scalar) const { return Radian(value / scalar); }

    constexpr Radian& operator+=(NumType scalar) { value += scalar; return *this; }
    constexpr Radian& operator-=(NumType scalar) { value -= scalar; return *this; }
    constexpr Radian& operator*=(NumType scalar) { value *= scalar; return *this; }
    constexpr Radian& operator/=(NumType scalar) { value /= scalar; return *this; }

    [[nodiscard]] constexpr Radian operator-() const { return Radian(-value); }

    [[nodiscard]] friend constexpr Radian operator+(NumType scalar, const Radian& deg) { return Radian(scalar + deg.value); }
    [[nodiscard]] friend constexpr Radian operator-(NumType scalar, const Radian& deg) { return Radian(scalar - deg.value); }
    [[nodiscard]] friend constexpr Radian operator*(NumType scalar, const Radian& deg) { return Radian(scalar * deg.value); }
    [[nodiscard]] friend constexpr Radian operator/(NumType scalar, const Radian& deg) { return Radian(scalar / deg.value); }

    [[nodiscard]] constexpr bool operator==(const Radian& rhs) const = default;
    [[nodiscard]] constexpr auto operator<=>(const Radian& rhs) const = default;

    [[nodiscard]] constexpr NumType operator*() const { return value; }
    [[nodiscard]] explicit constexpr operator NumType() const { return value; }
    [[nodiscard]] explicit constexpr operator Degree<NumType>() const { return Degree<NumType>(*this); }
};


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
