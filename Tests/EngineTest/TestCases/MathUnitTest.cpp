#include "doctest.h"

import SE.Prelude;
import std;


TEST_SUITE("SimpleEngine.Math:Literals")
{
TEST_CASE("AngleType - Construction and Literals")
{
    SUBCASE("Direct construction")
    {
        Degree<double> d(90.0);
        CHECK(d.value == 90.0);

        Radian<float> r(1.57f);
        CHECK(r.value == 1.57f);
    }

    SUBCASE("User-defined literals")
    {
        auto d_double = 180.0_deg;
        CHECK(d_double.value == 180.0);
        CHECK(std::same_as<decltype(d_double), Degree<double>>);

        auto d_ull = 90_deg;
        CHECK(d_ull.value == 90.0);
        CHECK(std::same_as<decltype(d_ull), Degree<double>>);

        auto r_float = 3.14_radf;
        CHECK(r_float.value == 3.14f);
        CHECK(std::same_as<decltype(r_float), Radian<float>>);
    }

    SUBCASE("Construction from different floating type")
    {
        Degree<float> d_float(45.0f);
        Degree<double> d_double(d_float);
        CHECK(d_double.value == doctest::Approx(45.0));
    }
}

TEST_CASE("AngleType - Unit Conversions")
{
    SUBCASE("Degree to Radian")
    {
        auto deg = 90.0_deg;
        auto rad = Radian<double>(deg); // 명시적 생성자 호출

        CHECK(rad.value == doctest::Approx(std::numbers::pi / 2.0));
    }

    SUBCASE("Radian to Degree")
    {
        auto rad = Radian<float>(std::numbers::pi_v<float>);
        auto deg = Degree<float>(rad); // 명시적 생성자 호출

        CHECK(deg.value == doctest::Approx(180.0f));
    }

    SUBCASE("Explicit cast operator")
    {
        auto deg = 180.0_deg;
        auto rad = static_cast<Radian<double>>(deg);

        CHECK(rad.value == doctest::Approx(std::numbers::pi));
    }

    SUBCASE("Conversion to underlying numeric type")
    {
        auto deg = 45.0_deg;
        double value = static_cast<double>(deg);
        CHECK(value == 45.0);

        double value_from_op = *deg;
        CHECK(value_from_op == 45.0);
    }
}

TEST_CASE("AngleType - Arithmetic Operations")
{
    auto d1 = 90.0_deg;
    auto d2 = 45.0_deg;

    SUBCASE("Angle + Angle")
    {
        auto result = d1 + d2;
        CHECK(result.value == doctest::Approx(135.0));
        CHECK(std::same_as<decltype(result), Degree<double>>);
    }

    SUBCASE("Angle - Angle")
    {
        auto result = d1 - d2;
        CHECK(result.value == doctest::Approx(45.0));
    }

    SUBCASE("Angle * scalar")
    {
        auto result = d1 * 2.0;
        CHECK(result.value == doctest::Approx(180.0));
    }

    SUBCASE("scalar * Angle")
    {
        auto result = 0.5 * d1;
        CHECK(result.value == doctest::Approx(45.0));
    }

    SUBCASE("Unary minus")
    {
        auto result = -d1;
        CHECK(result.value == doctest::Approx(-90.0));
    }

    SUBCASE("Compound assignment")
    {
        auto d_copy = d1;
        d_copy += d2;
        CHECK(d_copy.value == doctest::Approx(135.0));

        d_copy -= 90.0_deg;
        CHECK(d_copy.value == doctest::Approx(45.0));

        d_copy *= 4.0;
        CHECK(d_copy.value == doctest::Approx(180.0));
    }
}

TEST_CASE("AngleType - Comparison Operations")
{
    auto d1 = 90.0_deg;
    auto d2 = 90.0_deg;
    auto d3 = 180.0_deg;

    CHECK(d1 == d2);
    CHECK(d1 != d3);
    CHECK(d1 < d3);
    CHECK(d3 > d2);
    CHECK(d1 <= d2);
    CHECK(d1 <= d3);
    CHECK(d3 >= d1);
}

TEST_CASE("AngleType - Constexpr Evaluation")
{
    constexpr auto deg_at_compile_time = 90.0_degf;
    constexpr auto rad_at_compile_time = Radian<float>(deg_at_compile_time);
    constexpr auto calculated_at_compile_time = rad_at_compile_time * 2.0f;

    // Check if the final value is correct
    CHECK(calculated_at_compile_time.value == doctest::Approx(std::numbers::pi_v<float>));

    // Statically assert to be absolutely sure it's constexpr
    static_assert(calculated_at_compile_time.value > 3.14f && calculated_at_compile_time.value < 3.15f);
}
}
