#include "gtest/gtest.h"
#include "SimpleEngine/Core/Reflection/Enum.h"
#include <cstdint>

namespace EnumTestDetail
{
    enum class EColor { Red, Green, Blue };
    enum class ENumbers { One = 1, Five = 5, Ten = 10 };
    enum class EDirection { Up, Down, Left, Right };
    enum class ESparse : int8_t { Neg = -5, Zero = 0, Pos = 5 };
}

class EnumTest : public ::testing::Test {};

TEST_F(EnumTest, EnumNameCompileTime)
{
    using namespace EnumTestDetail;
    using namespace se;

    constexpr auto name1 = EnumName<EColor::Red>();
    EXPECT_EQ(name1, "Red");

    constexpr auto name2 = EnumName<EColor::Green>();
    EXPECT_EQ(name2, "Green");

    constexpr auto name3 = EnumName<EColor::Blue>();
    EXPECT_EQ(name3, "Blue");

    constexpr auto name4 = EnumName<ENumbers::One>();
    EXPECT_EQ(name4, "One");

    constexpr auto name5 = EnumName<ESparse::Neg>();
    EXPECT_EQ(name5, "Neg");
}

TEST_F(EnumTest, EnumNameRuntime)
{
    using namespace EnumTestDetail;
    using namespace se;

    EXPECT_EQ(EnumName(EColor::Red), "Red");
    EXPECT_EQ(EnumName(EColor::Green), "Green");
    EXPECT_EQ(EnumName(EColor::Blue), "Blue");

    EXPECT_EQ(EnumName(ENumbers::Five), "Five");
    EXPECT_EQ(EnumName(ESparse::Zero), "Zero");

    // Invalid value (out of range/not defined in enum)
    // Note: If casted value is within -128 to 127 but not defined in enum, name should be empty.
    EXPECT_TRUE(EnumName(static_cast<EColor>(99)).IsEmpty());
}

TEST_F(EnumTest, EnumCast)
{
    using namespace EnumTestDetail;
    using namespace se;

    auto val1 = EnumCast<EColor>("Red");
    ASSERT_TRUE(val1.HasValue());
    EXPECT_EQ(*val1, EColor::Red);

    auto val2 = EnumCast<EColor>("Blue");
    ASSERT_TRUE(val2.HasValue());
    EXPECT_EQ(*val2, EColor::Blue);

    auto val3 = EnumCast<ENumbers>("Ten");
    ASSERT_TRUE(val3.HasValue());
    EXPECT_EQ(*val3, ENumbers::Ten);

    auto val4 = EnumCast<EColor>("Purple"); // Invalid name
    EXPECT_FALSE(val4.HasValue());
}

TEST_F(EnumTest, EnumValues)
{
    using namespace EnumTestDetail;
    using namespace se;

    const auto& values = EnumValues<EColor>();
    EXPECT_EQ(values.Len(), 3);
    EXPECT_EQ(values[0], EColor::Red);
    EXPECT_EQ(values[1], EColor::Green);
    EXPECT_EQ(values[2], EColor::Blue);

    const auto& sparseValues = EnumValues<ESparse>();
    EXPECT_EQ(sparseValues.Len(), 3);
    // Order depends on the scan order (usually increasing integer value)
    // -5, 0, 5
    EXPECT_EQ(sparseValues[0], ESparse::Neg);
    EXPECT_EQ(sparseValues[1], ESparse::Zero);
    EXPECT_EQ(sparseValues[2], ESparse::Pos);
}

TEST_F(EnumTest, EnumNames)
{
    using namespace EnumTestDetail;
    using namespace se;

    const auto& names = EnumNames<EColor>();
    EXPECT_EQ(names.Len(), 3);
    EXPECT_EQ(names[0], "Red");
    EXPECT_EQ(names[1], "Green");
    EXPECT_EQ(names[2], "Blue");

    const auto& sparseNames = EnumNames<ESparse>();
    EXPECT_EQ(sparseNames.Len(), 3);
    EXPECT_EQ(sparseNames[0], "Neg");
    EXPECT_EQ(sparseNames[1], "Zero");
    EXPECT_EQ(sparseNames[2], "Pos");
}

TEST_F(EnumTest, EnumCount)
{
    using namespace EnumTestDetail;
    using namespace se;

    EXPECT_EQ(EnumCount<EColor>(), 3);
    EXPECT_EQ(EnumCount<ENumbers>(), 3);
    EXPECT_EQ(EnumCount<EDirection>(), 4);
    EXPECT_EQ(EnumCount<ESparse>(), 3);
}

TEST_F(EnumTest, EnumLoopIteration)
{
    using namespace EnumTestDetail;
    using namespace se;

    int count = 0;
    for (auto val : EnumValues<EColor>())
    {
        StringView name = EnumName(val);
        EXPECT_FALSE(name.IsEmpty());
        count++;
    }
    EXPECT_EQ(count, 3);
}
