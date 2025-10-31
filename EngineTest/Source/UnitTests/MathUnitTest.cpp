#include "gtest/gtest.h"
#include "SimpleEngine/Core/Math/Math.h"

class MathLiteralsTest : public ::testing::Test{};
class MathMatrix4x4Test : public ::testing::Test{};


TEST_F(MathLiteralsTest, AngleType_ConstructionAndLiterals)
{
    // Direct construction
    {
        Degree<double> d(90.0);
        EXPECT_EQ(d.value, 90.0);

        Radian<float> r(1.57f);
        EXPECT_EQ(r.value, 1.57f);
    }

    // User-defined literals
    {
        auto d_double = 180.0_deg;
        EXPECT_EQ(d_double.value, 180.0);
        static_assert(std::same_as<decltype(d_double), Degree<double>>);

        auto d_ull = 90_deg;
        EXPECT_EQ(d_ull.value, 90.0);
        static_assert(std::same_as<decltype(d_ull), Degree<double>>);

        auto r_float = 3.14_radf;
        EXPECT_EQ(r_float.value, 3.14f);
        static_assert(std::same_as<decltype(r_float), Radian<float>>);
    }

    // Construction from different floating type
    {
        Degree<float> d_float(45.0f);
        Degree<double> d_double(d_float);
        EXPECT_DOUBLE_EQ(d_double.value, 45.0);
    }
}

TEST_F(MathLiteralsTest, AngleType_UnitConversions)
{
    // Degree to Radian
    {
        auto deg = 90.0_deg;
        auto rad = Radian<double>(deg); // 명시적 생성자 호출

        EXPECT_DOUBLE_EQ(rad.value, std::numbers::pi / 2.0);
    }

    // Radian to Degree
    {
        auto rad = Radian<float>(std::numbers::pi_v<float>);
        auto deg = Degree<float>(rad); // 명시적 생성자 호출

        EXPECT_FLOAT_EQ(deg.value, 180.0f);
    }

    // Explicit cast operator
    {
        auto deg = 180.0_deg;
        auto rad = static_cast<Radian<double>>(deg);

        EXPECT_DOUBLE_EQ(rad.value, std::numbers::pi);
    }

    // Conversion to underlying numeric type
    {
        auto deg = 45.0_deg;
        double value = static_cast<double>(deg);
        EXPECT_DOUBLE_EQ(value, 45.0);

        double value_from_op = *deg;
        EXPECT_DOUBLE_EQ(value_from_op, 45.0);
    }
}

TEST_F(MathLiteralsTest, AngleType_ArithmeticOperations)
{
    auto d1 = 90.0_deg;
    auto d2 = 45.0_deg;

    // Angle + Angle
    {
        auto result = d1 + d2;
        EXPECT_DOUBLE_EQ(result.value, 135.0);
        static_assert(std::same_as<decltype(result), Degree<double>>);
    }

    // Angle - Angle
    {
        auto result = d1 - d2;
        EXPECT_DOUBLE_EQ(result.value, 45.0);
    }

    // Angle * scalar
    {
        auto result = d1 * 2.0;
        EXPECT_DOUBLE_EQ(result.value, 180.0);
    }

    // scalar * Angle
    {
        auto result = 0.5 * d1;
        EXPECT_DOUBLE_EQ(result.value, 45.0);
    }

    // Unary minus
    {
        auto result = -d1;
        EXPECT_DOUBLE_EQ(result.value, -90.0);
    }

    // Compound assignment
    {
        auto d_copy = d1;
        d_copy += d2;
        EXPECT_DOUBLE_EQ(d_copy.value, 135.0);

        d_copy -= 90.0_deg;
        EXPECT_DOUBLE_EQ(d_copy.value, 45.0);

        d_copy *= 4.0;
        EXPECT_DOUBLE_EQ(d_copy.value, 180.0);
    }
}

TEST_F(MathLiteralsTest, AngleType_ComparisonOperations)
{
    auto d1 = 90.0_deg;
    auto d2 = 90.0_deg;
    auto d3 = 180.0_deg;

    EXPECT_EQ(d1, d2);
    EXPECT_NE(d1, d3);
    EXPECT_LT(d1, d3);
    EXPECT_GT(d3, d2);
    EXPECT_LE(d1, d2);
    EXPECT_LE(d1, d3);
    EXPECT_GE(d3, d1);
}

TEST_F(MathLiteralsTest, AngleType_ConstexprEvaluation)
{
    constexpr auto deg_at_compile_time = 90.0_degf;
    constexpr auto rad_at_compile_time = Radian<float>(deg_at_compile_time);
    constexpr auto calculated_at_compile_time = rad_at_compile_time * 2.0f;

    // Check if the final value is correct
    EXPECT_FLOAT_EQ(calculated_at_compile_time.value, std::numbers::pi_v<float>);

    // Statically assert to be absolutely sure it's constexpr
    static_assert(calculated_at_compile_time.value > 3.14f && calculated_at_compile_time.value < 3.15f);
}


template <typename T>
void test_matrix_creation_and_zero()
{
    se::math::Matrix4x4Impl<T> mat{};
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_DOUBLE_EQ((mat[i, j]), T{0});
        }
    }

    se::math::Matrix4x4Impl<T> zero_mat = se::math::Matrix4x4Impl<T>::Zero();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_DOUBLE_EQ((zero_mat[i, j]), T{0});
        }
    }
}

TEST_F(MathMatrix4x4Test, float_CreationAndZero)
{
    test_matrix_creation_and_zero<float>();
}

TEST_F(MathMatrix4x4Test, double_CreationAndZero)
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

    EXPECT_DOUBLE_EQ((mat[0, 0]), static_cast<T>(1.0));
    EXPECT_DOUBLE_EQ((mat[0, 1]), static_cast<T>(2.0));
    EXPECT_DOUBLE_EQ((mat[3, 3]), static_cast<T>(16.0));
    EXPECT_DOUBLE_EQ((mat[1, 2]), static_cast<T>(7.0));
}

TEST_F(MathMatrix4x4Test, float_VariadicConstructor)
{
    test_matrix_variadic_constructor<float>();
}

TEST_F(MathMatrix4x4Test, double_VariadicConstructor)
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
                EXPECT_DOUBLE_EQ((identity_mat[i, j]), static_cast<T>(1.0));
            }
            else
            {
                EXPECT_DOUBLE_EQ((identity_mat[i, j]), static_cast<T>(0.0));
            }
        }
    }
}

TEST_F(MathMatrix4x4Test, float_IdentityMatrix)
{
    test_matrix_identity_matrix<float>();
}

TEST_F(MathMatrix4x4Test, double_IdentityMatrix)
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

    EXPECT_DOUBLE_EQ((transposed_mat[0, 0]), static_cast<T>(1.0));
    EXPECT_DOUBLE_EQ((transposed_mat[0, 1]), static_cast<T>(5.0));
    EXPECT_DOUBLE_EQ((transposed_mat[1, 0]), static_cast<T>(2.0));
    EXPECT_DOUBLE_EQ((transposed_mat[3, 2]), static_cast<T>(12.0));

    EXPECT_DOUBLE_EQ((transposed_mat[2, 3]), static_cast<T>(15.0));
}

TEST_F(MathMatrix4x4Test, float_Transpose)
{
    test_matrix_transpose<float>();
}

TEST_F(MathMatrix4x4Test, double_Transpose)
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

    EXPECT_DOUBLE_EQ((result[0, 0]), static_cast<T>(2.0));
    EXPECT_DOUBLE_EQ((result[0, 1]), static_cast<T>(3.0));
    EXPECT_DOUBLE_EQ((result[3, 3]), static_cast<T>(17.0));

    mat1 += mat2;
    EXPECT_DOUBLE_EQ((mat1[0, 0]), static_cast<T>(2.0));
    EXPECT_DOUBLE_EQ((mat1[0, 1]), static_cast<T>(3.0));
    EXPECT_DOUBLE_EQ((mat1[3, 3]), static_cast<T>(17.0));
}

TEST_F(MathMatrix4x4Test, float_Addition)
{
    test_matrix_addition<float>();
}

TEST_F(MathMatrix4x4Test, double_Addition)
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

    EXPECT_DOUBLE_EQ((result[0, 0]), static_cast<T>(2.0));
    EXPECT_DOUBLE_EQ((result[0, 1]), static_cast<T>(4.0));
    EXPECT_DOUBLE_EQ((result[3, 3]), static_cast<T>(32.0));

    mat *= static_cast<T>(0.5);
    EXPECT_DOUBLE_EQ((mat[0, 0]), static_cast<T>(0.5));
    EXPECT_DOUBLE_EQ((mat[0, 1]), static_cast<T>(1.0));
    EXPECT_DOUBLE_EQ((mat[3, 3]), static_cast<T>(8.0));
}

TEST_F(MathMatrix4x4Test, float_ScalarMultiplication)
{
    test_matrix_scalar_multiplication<float>();
}

TEST_F(MathMatrix4x4Test, double_ScalarMultiplication)
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
            EXPECT_DOUBLE_EQ((result[i, j]), (mat2[i, j]));
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
            EXPECT_DOUBLE_EQ((result[i, j]), (mat4_expected[i, j]));
        }
    }

    se::math::Matrix4x4Impl<T> original_mat2 = mat2;
    mat2 *= mat1;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_DOUBLE_EQ((mat2[i, j]), (original_mat2[i, j]));
        }
    }
}

TEST_F(MathMatrix4x4Test, float_MatrixMultiplication)
{
    test_matrix_multiplication<float>();
}

TEST_F(MathMatrix4x4Test, double_MatrixMultiplication)
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

    EXPECT_DOUBLE_EQ((result[0]), static_cast<T>(10.0));
    EXPECT_DOUBLE_EQ((result[1]), static_cast<T>(20.0));
    EXPECT_DOUBLE_EQ((result[2]), static_cast<T>(30.0));
    EXPECT_DOUBLE_EQ((result[3]), static_cast<T>(1.0));

    se::math::Matrix4x4Impl<T> translation_mat(
        static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(0.0),
        static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0),
        static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0),
        static_cast<T>(100.0), static_cast<T>(200.0), static_cast<T>(300.0), static_cast<T>(1.0)
    );

    se::math::Vector4Impl<T> vec_to_translate(static_cast<T>(1.0), static_cast<T>(2.0), static_cast<T>(3.0), static_cast<T>(1.0));
    se::math::Vector4Impl<T> translated_vec = vec_to_translate * translation_mat;

    EXPECT_DOUBLE_EQ((translated_vec[0]), static_cast<T>(1.0) + static_cast<T>(100.0));
    EXPECT_DOUBLE_EQ((translated_vec[1]), static_cast<T>(2.0) + static_cast<T>(200.0));
    EXPECT_DOUBLE_EQ((translated_vec[2]), static_cast<T>(3.0) + static_cast<T>(300.0));
    EXPECT_DOUBLE_EQ((translated_vec[3]), static_cast<T>(1.0));
}

TEST_F(MathMatrix4x4Test, float_Vector4Multiplication)
{
    test_matrix_vector4_multiplication<float>();
}

TEST_F(MathMatrix4x4Test, double_Vector4Multiplication)
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
                EXPECT_DOUBLE_EQ((inverse_identity[i, j]), static_cast<T>(1.0));
            }
            else
            {
                EXPECT_DOUBLE_EQ((inverse_identity[i, j]), static_cast<T>(0.0));
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
                EXPECT_DOUBLE_EQ((product[i, j]), static_cast<T>(1.0));
            }
            else
            {
                EXPECT_DOUBLE_EQ((product[i, j]), static_cast<T>(0.0));
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
            EXPECT_DOUBLE_EQ((calculated_inverse[i, j]), (expected_inverse[i, j]));
        }
    }

    product = mat_to_inverse * calculated_inverse;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            if (i == j)
            {
                EXPECT_DOUBLE_EQ((product[i, j]), static_cast<T>(1.0));
            }
            else
            {
                EXPECT_DOUBLE_EQ((product[i, j]), static_cast<T>(0.0));
            }
        }
    }
}

TEST_F(MathMatrix4x4Test, float_Inverse)
{
    test_matrix_inverse<float>();
}

TEST_F(MathMatrix4x4Test, double_Inverse)
{
    test_matrix_inverse<double>();
}
