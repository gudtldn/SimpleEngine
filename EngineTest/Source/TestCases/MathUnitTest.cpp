#include "doctest/doctest.h"
#include "SimpleEngine/Core/Math/Math.h"


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

TEST_SUITE("SimpleEngine.Math:Matrix4x4")
{
    template <typename T>
    void test_matrix_creation_and_zero()
    {
        se::math::Matrix4x4Impl<T> mat{};
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                CHECK(mat[i, j] == doctest::Approx(T{0}));
            }
        }

        se::math::Matrix4x4Impl<T> zero_mat = se::math::Matrix4x4Impl<T>::Zero();
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                CHECK(zero_mat[i, j] == doctest::Approx(T{0}));
            }
        }
    }

    TEST_CASE("Matrix4x4Impl: float - Creation and Zero")
    {
        test_matrix_creation_and_zero<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Creation and Zero")
    {
        test_matrix_creation_and_zero<double>();
    }

    template <typename T>
    void test_matrix_variadic_constructor()
    {
        se::math::Matrix4x4Impl<T> mat(
            static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(4.0),
            static_cast<T>(5.0), static_cast<T>(6.0), static_cast<T>(7.0), static_cast<T>(8.0),
            static_cast<T>(9.0), static_cast<T>(10.0), static_cast<T>(11.0), static_cast<T>(12.0),
            static_cast<T>(13.0), static_cast<T>(14.0), static_cast<T>(15.0), static_cast<T>(16.0)
        );

        CHECK(mat[0, 0] == doctest::Approx(static_cast<T>(1.0)));
        CHECK(mat[0, 1] == doctest::Approx(static_cast<T>(2.0)));
        CHECK(mat[3, 3] == doctest::Approx(static_cast<T>(16.0)));
        CHECK(mat[1, 2] == doctest::Approx(static_cast<T>(7.0)));
    }

    TEST_CASE("Matrix4x4Impl: float - Variadic Constructor")
    {
        test_matrix_variadic_constructor<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Variadic Constructor")
    {
        test_matrix_variadic_constructor<double>();
    }

    template <typename T>
    void test_matrix_identity_matrix()
    {
        se::math::Matrix4x4Impl<T> identity_mat = se::math::Matrix4x4Impl<T>::Identity();
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (i == j)
                {
                    CHECK(identity_mat[i, j] == doctest::Approx(static_cast<T>(1.0)));
                }
                else
                {
                    CHECK(identity_mat[i, j] == doctest::Approx(static_cast<T>(0.0)));
                }
            }
        }
    }

    TEST_CASE("Matrix4x4Impl: float - Identity Matrix")
    {
        test_matrix_identity_matrix<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Identity Matrix")
    {
        test_matrix_identity_matrix<double>();
    }

    template <typename T>
    void test_matrix_transpose()
    {
        se::math::Matrix4x4Impl<T> mat(
            static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(4.0),
            static_cast<T>(5.0), static_cast<T>(6.0), static_cast<T>(7.0), static_cast<T>(8.0),
            static_cast<T>(9.0), static_cast<T>(10.0), static_cast<T>(11.0), static_cast<T>(12.0),
            static_cast<T>(13.0), static_cast<T>(14.0), static_cast<T>(15.0), static_cast<T>(16.0)
        );

        se::math::Matrix4x4Impl<T> transposed_mat = mat.Transpose();

        CHECK(transposed_mat[0, 0] == doctest::Approx(static_cast<T>(1.0)));
        CHECK(transposed_mat[0, 1] == doctest::Approx(static_cast<T>(5.0)));
        CHECK(transposed_mat[1, 0] == doctest::Approx(static_cast<T>(2.0)));
        CHECK(transposed_mat[3, 2] == doctest::Approx(static_cast<T>(12.0)));
        CHECK(transposed_mat[2, 3] == doctest::Approx(static_cast<T>(15.0)));
    }

    TEST_CASE("Matrix4x4Impl: float - Transpose")
    {
        test_matrix_transpose<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Transpose")
    {
        test_matrix_transpose<double>();
    }

    template <typename T>
    void test_matrix_addition()
    {
        se::math::Matrix4x4Impl<T> mat1(
            static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(4.0),
            static_cast<T>(5.0), static_cast<T>(6.0), static_cast<T>(7.0), static_cast<T>(8.0),
            static_cast<T>(9.0), static_cast<T>(10.0), static_cast<T>(11.0), static_cast<T>(12.0),
            static_cast<T>(13.0), static_cast<T>(14.0), static_cast<T>(15.0), static_cast<T>(16.0)
        );

        se::math::Matrix4x4Impl<T> mat2(
            static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0),
            static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0),
            static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0),
            static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0), static_cast<T>(1.0)
        );

        se::math::Matrix4x4Impl<T> result = mat1 + mat2;

        CHECK(result[0, 0] == doctest::Approx(static_cast<T>(2.0)));
        CHECK(result[0, 1] == doctest::Approx(static_cast<T>(3.0)));
        CHECK(result[3, 3] == doctest::Approx(static_cast<T>(17.0)));

        mat1 += mat2;
        CHECK(mat1[0, 0] == doctest::Approx(static_cast<T>(2.0)));
        CHECK(mat1[0, 1] == doctest::Approx(static_cast<T>(3.0)));
        CHECK(mat1[3, 3] == doctest::Approx(static_cast<T>(17.0)));
    }

    TEST_CASE("Matrix4x4Impl: float - Addition")
    {
        test_matrix_addition<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Addition")
    {
        test_matrix_addition<double>();
    }

    template <typename T>
    void test_matrix_scalar_multiplication()
    {
        se::math::Matrix4x4Impl<T> mat(
            static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(4.0),
            static_cast<T>(5.0), static_cast<T>(6.0), static_cast<T>(7.0), static_cast<T>(8.0),
            static_cast<T>(9.0), static_cast<T>(10.0), static_cast<T>(11.0), static_cast<T>(12.0),
            static_cast<T>(13.0), static_cast<T>(14.0), static_cast<T>(15.0), static_cast<T>(16.0)
        );

        se::math::Matrix4x4Impl<T> result = mat * static_cast<T>(2.0);

        CHECK(result[0, 0] == doctest::Approx(static_cast<T>(2.0)));
        CHECK(result[0, 1] == doctest::Approx(static_cast<T>(4.0)));
        CHECK(result[3, 3] == doctest::Approx(static_cast<T>(32.0)));

        mat *= static_cast<T>(0.5);
        CHECK(mat[0, 0] == doctest::Approx(static_cast<T>(0.5)));
        CHECK(mat[0, 1] == doctest::Approx(static_cast<T>(1.0)));
        CHECK(mat[3, 3] == doctest::Approx(static_cast<T>(8.0)));
    }

    TEST_CASE("Matrix4x4Impl: float - Scalar Multiplication")
    {
        test_matrix_scalar_multiplication<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Scalar Multiplication")
    {
        test_matrix_scalar_multiplication<double>();
    }

    template <typename T>
    void test_matrix_multiplication()
    {
        se::math::Matrix4x4Impl<T> mat1(
            static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)
        );

        se::math::Matrix4x4Impl<T> mat2(
            static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(4.0),
            static_cast<T>(5.0), static_cast<T>(6.0), static_cast<T>(7.0), static_cast<T>(8.0),
            static_cast<T>(9.0), static_cast<T>(10.0), static_cast<T>(11.0), static_cast<T>(12.0),
            static_cast<T>(13.0), static_cast<T>(14.0), static_cast<T>(15.0), static_cast<T>(16.0)
        );

        se::math::Matrix4x4Impl<T> result = mat1 * mat2;

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                CHECK(result[i, j] == doctest::Approx(mat2[i, j]));
            }
        }

        se::math::Matrix4x4Impl<T> mat3(
            static_cast<T>(2.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(2.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(2.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(2.0)
        );

        se::math::Matrix4x4Impl<T> mat4_expected(
            static_cast<T>(2.0), static_cast<T>(4.0), static_cast<T>(6.0), static_cast<T>(8.0),
            static_cast<T>(10.0), static_cast<T>(12.0), static_cast<T>(14.0), static_cast<T>(16.0),
            static_cast<T>(18.0), static_cast<T>(20.0), static_cast<T>(22.0), static_cast<T>(24.0),
            static_cast<T>(26.0), static_cast<T>(28.0), static_cast<T>(30.0), static_cast<T>(32.0)
        );

        result = mat3 * mat2;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                CHECK(result[i, j] == doctest::Approx(mat4_expected[i, j]));
            }
        }

        se::math::Matrix4x4Impl<T> original_mat2 = mat2;
        mat2 *= mat1;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                CHECK(mat2[i, j] == doctest::Approx(original_mat2[i, j]));
            }
        }
    }

    TEST_CASE("Matrix4x4Impl: float - Matrix Multiplication")
    {
        test_matrix_multiplication<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Matrix Multiplication")
    {
        test_matrix_multiplication<double>();
    }

    template <typename T>
    void test_matrix_vector4_multiplication()
    {
        se::math::Matrix4x4Impl<T> mat(
            static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)
        );

        se::math::Vector4Impl<T> vec(static_cast<T>(10.0), static_cast<T>(20.0), static_cast<T>(30.0), static_cast<T>(1.0));

        se::math::Vector4Impl<T> result = vec * mat;

        CHECK(result[0] == doctest::Approx(static_cast<T>(10.0)));
        CHECK(result[1] == doctest::Approx(static_cast<T>(20.0)));
        CHECK(result[2] == doctest::Approx(static_cast<T>(30.0)));
        CHECK(result[3] == doctest::Approx(static_cast<T>(1.0)));

        se::math::Matrix4x4Impl<T> translation_mat(
            static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0),
            static_cast<T>(100.0), static_cast<T>(200.0), static_cast<T>(300.0), static_cast<T>(1.0)
        );

        se::math::Vector4Impl<T> vec_to_translate(static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(1.0));
        se::math::Vector4Impl<T> translated_vec = vec_to_translate * translation_mat;

        CHECK(translated_vec[0] == doctest::Approx(static_cast<T>(1.0) + static_cast<T>(100.0)));
        CHECK(translated_vec[1] == doctest::Approx(static_cast<T>(2.0) + static_cast<T>(200.0)));
        CHECK(translated_vec[2] == doctest::Approx(static_cast<T>(3.0) + static_cast<T>(300.0)));
        CHECK(translated_vec[3] == doctest::Approx(static_cast<T>(1.0)));
    }

    TEST_CASE("Matrix4x4Impl: float - Vector4 Multiplication")
    {
        test_matrix_vector4_multiplication<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Vector4 Multiplication")
    {
        test_matrix_vector4_multiplication<double>();
    }

    template <typename T>
    void test_matrix_inverse()
    {
        se::math::Matrix4x4Impl<T> identity_mat = se::math::Matrix4x4Impl<T>::Identity();
        se::math::Matrix4x4Impl<T> inverse_identity = identity_mat.Inverse();

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (i == j)
                {
                    CHECK(inverse_identity[i, j] == doctest::Approx(static_cast<T>(1.0)));
                }
                else
                {
                    CHECK(inverse_identity[i, j] == doctest::Approx(static_cast<T>(0.0)));
                }
            }
        }

        se::math::Matrix4x4Impl<T> product = identity_mat * inverse_identity;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (i == j)
                {
                    CHECK(product[i, j] == doctest::Approx(static_cast<T>(1.0)));
                }
                else
                {
                    CHECK(product[i, j] == doctest::Approx(static_cast<T>(0.0)));
                }
            }
        }

        se::math::Matrix4x4Impl<T> mat_to_inverse(
            static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(4.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)
        );
        se::math::Matrix4x4Impl<T> expected_inverse(
            static_cast<T>(1.0), static_cast<T>(-2.0), static_cast<T>(5.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(-4.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0),
            static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)
        );

        se::math::Matrix4x4Impl<T> calculated_inverse = mat_to_inverse.Inverse();

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                CHECK(calculated_inverse[i, j] == doctest::Approx(expected_inverse[i, j]));
            }
        }

        product = mat_to_inverse * calculated_inverse;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (i == j)
                {
                    CHECK(product[i, j] == doctest::Approx(static_cast<T>(1.0)));
                }
                else
                {
                    CHECK(product[i, j] == doctest::Approx(static_cast<T>(0.0)));
                }
            }
        }
    }

    TEST_CASE("Matrix4x4Impl: float - Inverse")
    {
        test_matrix_inverse<float>();
    }

    TEST_CASE("Matrix4x4Impl: double - Inverse")
    {
        test_matrix_inverse<double>();
    }
}
