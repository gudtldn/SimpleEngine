#include "gtest/gtest.h"
#include "SimpleEngine/Math/Random.h"

#include <thread>

using namespace se::math;

TEST(RandomTest, Stream_Consistency)
{
    RandomStream rs1(12345, 67890);
    RandomStream rs2(12345, 67890);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(rs1.Next(), rs2.Next());
    }
}

TEST(RandomTest, Stream_DifferentSeed)
{
    RandomStream rs1(12345, 67890);
    RandomStream rs2(54321, 67890);

    // 시드가 다르면 결과도 달라야 함 (확률적으로 매우 높음)
    EXPECT_NE(rs1.Next(), rs2.Next());
}

TEST(RandomTest, Range_Int)
{
    RandomStream rs(42);
    for (int i = 0; i < 1000; ++i)
    {
        int32 val = rs.Range(-10, 10);
        EXPECT_GE(val, -10);
        EXPECT_LE(val, 10);
    }
}

TEST(RandomTest, Range_UInt)
{
    RandomStream rs(42);
    for (int i = 0; i < 1000; ++i)
    {
        uint32 val = rs.Range(100);
        EXPECT_LT(val, 100u);
    }
}

TEST(RandomTest, Float_Range)
{
    RandomStream rs(42);
    for (int i = 0; i < 1000; ++i)
    {
        float val = rs.Float();
        EXPECT_GE(val, 0.0f);
        EXPECT_LT(val, 1.0f);

        float r_val = rs.Range(-5.0f, 5.0f);
        EXPECT_GE(r_val, -5.0f);
        EXPECT_LT(r_val, 5.0f);
    }
}

TEST(RandomTest, Global_ThreadSafety)
{
    Random::Seed(42, 123);
    [[maybe_unused]] float val_main = Random::Float();

    std::thread t1([]()
    {
        Random::Seed(42, 123);
        float val_t1 = Random::Float();

        RandomStream rs(42, 123);
        EXPECT_EQ(val_t1, rs.Float());
    });

    t1.join();

    // 메인 스레드의 상태는 t1에 의해 변경되지 않아야 함
    float val_main_next = Random::Float();
    RandomStream rs_main(42, 123);
    rs_main.Float(); // val_main
    EXPECT_EQ(val_main_next, rs_main.Float());
}

TEST(RandomTest, Zero_Seed_Safety)
{
    // PCG는 0 시드에서도 안전하게 동작해야 함
    RandomStream rs(0, 0);
    uint32 v1 = rs.Next();
    uint32 v2 = rs.Next();
    uint32 v3 = rs.Next();

    EXPECT_NE(v1, v2);
    EXPECT_NE(v2, v3);
}
