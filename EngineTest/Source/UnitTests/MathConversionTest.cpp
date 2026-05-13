#include "gtest/gtest.h"
#include "SimpleEngine/Core/Math/Math.h"
#include <type_traits>

using namespace se;
using namespace se::math;

TEST(MathConversionTest, VectorConversion)
{
    // f32 -> f64 (Implicit)
    {
        Vector3f v3f{ 1.0f, 2.0f, 3.0f };
        Vector3 v3d = v3f;
        EXPECT_DOUBLE_EQ(v3d.x, 1.0);
        EXPECT_DOUBLE_EQ(v3d.y, 2.0);
        EXPECT_DOUBLE_EQ(v3d.z, 3.0);

        static_assert(std::is_convertible_v<Vector3f, Vector3>);
    }

    // f64 -> f32 (Explicit)
    {
        Vector3 v3d{ 10.0, 20.0, 30.0 };
        Vector3f v3f = static_cast<Vector3f>(v3d);
        EXPECT_FLOAT_EQ(v3f.x, 10.0f);
        EXPECT_FLOAT_EQ(v3f.y, 20.0f);
        EXPECT_FLOAT_EQ(v3f.z, 30.0f);

        static_assert(!std::is_convertible_v<Vector3, Vector3f>);
        static_assert(std::is_constructible_v<Vector3f, Vector3>);
    }

    // Vector2
    {
        Vector2f v2f{ 1.0f, 2.0f };
        Vector2 v2d = v2f;
        EXPECT_DOUBLE_EQ(v2d.x, 1.0);

        Vector2 v2d_src{ 5.0, 6.0 };
        Vector2f v2f_dst = static_cast<Vector2f>(v2d_src);
        EXPECT_FLOAT_EQ(v2f_dst.x, 5.0f);
    }

    // Vector4
    {
        Vector4f v4f{ 1.0f, 2.0f, 3.0f, 4.0f };
        Vector4 v4d = v4f;
        EXPECT_DOUBLE_EQ(v4d.w, 4.0);

        Vector4 v4d_src{ 10.0, 11.0, 12.0, 13.0 };
        Vector4f v4f_dst = static_cast<Vector4f>(v4d_src);
        EXPECT_FLOAT_EQ(v4f_dst.w, 13.0f);
    }
}

TEST(MathConversionTest, RotationConversion)
{
    // Quaternion
    {
        Quaternionf qf{ 0.0f, 0.0f, 0.0f, 1.0f };
        Quaternion qd = qf;
        EXPECT_DOUBLE_EQ(qd.w, 1.0);

        Quaternion qd_src{ 1.0, 2.0, 3.0, 4.0 };
        Quaternionf qf_dst = static_cast<Quaternionf>(qd_src);
        EXPECT_FLOAT_EQ(qf_dst.x, 1.0f);

        static_assert(std::is_convertible_v<Quaternionf, Quaternion>);
        static_assert(!std::is_convertible_v<Quaternion, Quaternionf>);
    }

    // Rotator
    {
        Rotatorf rf{ 10.0_degf, 20.0_degf, 30.0_degf };
        Rotator rd = rf;
        EXPECT_DOUBLE_EQ(rd.pitch.value, 10.0);

        Rotator rd_src{ 45.0_deg, 90.0_deg, 180.0_deg };
        Rotatorf rf_dst = static_cast<Rotatorf>(rd_src);
        EXPECT_FLOAT_EQ(rf_dst.yaw.value, 180.0f);

        static_assert(std::is_convertible_v<Rotatorf, Rotator>);
        static_assert(!std::is_convertible_v<Rotator, Rotatorf>);
    }
}

TEST(MathConversionTest, MatrixConversion)
{
    Matrix4x4f mf = Matrix4x4f::Identity();
    mf[0, 3] = 10.0f;

    Matrix4x4 md = mf;
    EXPECT_DOUBLE_EQ((md[0, 3]), 10.0);

    Matrix4x4 md_src = Matrix4x4::Identity();
    md_src[3, 3] = 2.0;
    Matrix4x4f mf_dst = static_cast<Matrix4x4f>(md_src);
    EXPECT_FLOAT_EQ((mf_dst[3, 3]), 2.0f);

    static_assert(std::is_convertible_v<Matrix4x4f, Matrix4x4>);
    static_assert(!std::is_convertible_v<Matrix4x4, Matrix4x4f>);
}

TEST(MathConversionTest, GeometryConversion)
{
    // AABB
    {
        AABBf af{ Vector3f::Zero(), Vector3f::One() };
        AABB ad = af;
        EXPECT_DOUBLE_EQ(ad.max.z, 1.0);

        AABB ad_src{ Vector3::Zero(), Vector3{10.0, 10.0, 10.0} };
        AABBf af_dst = static_cast<AABBf>(ad_src);
        EXPECT_FLOAT_EQ(af_dst.max.y, 10.0f);

        static_assert(std::is_convertible_v<AABBf, AABB>);
        static_assert(!std::is_convertible_v<AABB, AABBf>);
    }

    // Ray
    {
        Rayf rf{ Vector3f::Zero(), Vector3f::Up() };
        Ray rd = rf;
        EXPECT_DOUBLE_EQ(rd.direction.z, 1.0);

        Ray rd_src{ Vector3{1.0, 2.0, 3.0}, Vector3::Forward() };
        Rayf rf_dst = static_cast<Rayf>(rd_src);
        EXPECT_FLOAT_EQ(rf_dst.origin.x, 1.0f);

        static_assert(std::is_convertible_v<Rayf, Ray>);
        static_assert(!std::is_convertible_v<Ray, Rayf>);
    }
}
