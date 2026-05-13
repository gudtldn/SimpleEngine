#include "gtest/gtest.h"
#include "SimpleEngine/Core/Reflection/Enum.h"
#include <cstdint>

namespace EnumTestDetail
{
enum class EColor { Red, Green, Blue };
enum class ENumbers { One = 1, Five = 5, Ten = 10 };
enum class EDirection { Up, Down, Left, Right };
enum class ESparse : i8 { Neg = -5, Zero = 0, Pos = 5 };

enum class EAlias { Start = 0, Begin = 0, First = 0, End = 10 };
enum class EFlags : u32 { None = 0, Read = 1 << 0, Write = 1 << 1, All = Read | Write };
enum class EEmpty {};
enum class ELarge : i64 { Max = 9223372036854775807LL, Min = -9223372036854775807LL - 1 };
} // namespace EnumTestDetail

SE_ENUM_SET_RANGE(EnumTestDetail::EColor, 0, 2);
SE_ENUM_SET_RANGE(EnumTestDetail::ENumbers, 1, 10);
SE_ENUM_SET_RANGE(EnumTestDetail::EDirection, 0, 3);
SE_ENUM_SET_RANGE(EnumTestDetail::ESparse, -5, 5);

// 2. [EAlias] 중복 값 처리
// Alias는 값(0)이 같으므로, 리플렉션 시 이름은 'Start', 'Begin', 'First' 중 하나만 나옵니다.
// (컴파일러 구현에 따라 다름, 보통 먼저 정의된 것).
// 따라서 범위 탐색(0~10)을 하면 됩니다.
SE_ENUM_SET_RANGE(EnumTestDetail::EAlias, 0, 10);

// 3. [EFlags] 복합 플래그 처리
// 'All' 값(3)을 찾으려면 일반 범위(0~3)로 설정하거나,
// 명확하게 값들을 나열하고 싶다면 SET_VALUES를 씁니다.
// 여기선 간단하게 범위로 해결 가능합니다.
SE_ENUM_SET_RANGE(EnumTestDetail::EFlags, 0, 3);
// 또는 수동 지정: SE_ENUM_SET_VALUES(EnumTestDetail::EFlags, EnumTestDetail::EFlags::None, EnumTestDetail::EFlags::Read, EnumTestDetail::EFlags::Write, EnumTestDetail::EFlags::All);

// 4. [EEmpty] 빈 Enum
// 기본 범위(0~64) 내에 값이 없으므로 자동으로 Empty가 됩니다. 별도 설정 불필요.
// 명시적으로 하려면:
SE_ENUM_SET_RANGE(EnumTestDetail::EEmpty, 0, 0);

// 5. [ELarge] 64비트 범위 (핵심)
// 범위 탐색 절대 불가. 새로 만든 매크로로 필요한 값만 콕 집어줍니다.
SE_ENUM_SET_VALUES(EnumTestDetail::ELarge, EnumTestDetail::ELarge::Max, EnumTestDetail::ELarge::Min);

class EnumTest : public ::testing::Test {};

TEST_F(EnumTest, EnumNameCompileTime)
{
    using namespace EnumTestDetail;
    using namespace se;

    constexpr auto NAME1 = EnumName<EColor::Red>();
    EXPECT_EQ(NAME1, "Red");

    constexpr auto NAME2 = EnumName<EColor::Green>();
    EXPECT_EQ(NAME2, "Green");

    constexpr auto NAME3 = EnumName<EColor::Blue>();
    EXPECT_EQ(NAME3, "Blue");

    constexpr auto NAME4 = EnumName<ENumbers::One>();
    EXPECT_EQ(NAME4, "One");

    constexpr auto NAME5 = EnumName<ESparse::Neg>();
    EXPECT_EQ(NAME5, "Neg");
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

/**
 * 1. 중복 값(Alias) 처리 테스트
 * 시스템이 첫 번째 이름을 선택하는지, 아니면 모든 이름을 인지하는지 확인합니다.
 */
TEST_F(EnumTest, DuplicateValues)
{
    using namespace EnumTestDetail;
    using namespace se;

    // 값이 같을 때 EnumName이 어떤 것을 반환하는지 정의된 정책 확인
    // 보통 가장 먼저 정의된 것을 반환하거나, 컴파일러 환경에 따라 다를 수 있음
    [[maybe_unused]] auto name = EnumName(EAlias::Begin);
    EXPECT_FALSE(name.IsEmpty()); // "Start" in MSVC

    // EnumValues는 중복된 값을 어떻게 처리하는가? (보통 하나로 취급하거나 모두 포함)
    [[maybe_unused]] const auto& values = EnumValues<EAlias>();
    EXPECT_FALSE(values.IsEmpty()); // Len = 2 in MSVC
}

/**
 * 2. 비트 플래그 및 복합 값 테스트
 */
TEST_F(EnumTest, BitFlags)
{
    using namespace EnumTestDetail;
    using namespace se;

    // 조합된 값(Read | Write)이 Enum에 All로 정의되어 있을 때의 동작
    EXPECT_EQ(EnumName(EFlags::All), "All");

    // 정의되지 않은 조합 (정의되지 않은 비트 플래그)
    EFlags undefined = static_cast<EFlags>(0x5);
    EXPECT_TRUE(EnumName(undefined).IsEmpty());
}

/**
 * 3. 빈 Enum 및 극단적 범위 테스트
 */
TEST_F(EnumTest, EdgeCases)
{
    using namespace EnumTestDetail;
    using namespace se;

    // 빈 Enum 확인
    EXPECT_EQ(EnumCount<EEmpty>(), 0);
    EXPECT_TRUE(EnumValues<EEmpty>().IsEmpty());

    // 64비트 큰 값 처리 확인
    EXPECT_EQ(EnumName(ELarge::Max), "Max");
    EXPECT_EQ(EnumCast<ELarge>("Min"), ELarge::Min);
}

/**
 * 4. 문자열 대소문자 및 공백 처리 (EnumCast)
 */
TEST_F(EnumTest, EnumCastRobustness)
{
    using namespace EnumTestDetail;
    using namespace se;

    // 대소문자 구분 여부 확인 (엔진 정책에 따라)
    auto valLower = EnumCast<EColor>("red");
    // 만약 Case-Insensitive 하다면 HasValue()가 true여야 함
    // 보통은 엄격하게 false인 경우가 많음
    EXPECT_FALSE(valLower.HasValue());

    // 앞뒤 공백이 포함된 경우
    auto valSpace = EnumCast<EColor>(" Red ");
    EXPECT_FALSE(valSpace.HasValue());
}

/**
 * 5. 연속되지 않은 음수 범위 (Sparse Negative)
 */
TEST_F(EnumTest, SparseNegativeRange)
{
    using namespace EnumTestDetail;
    using namespace se;

    // 음수 끝단 값 확인
    EXPECT_EQ(EnumName(ESparse::Neg), "Neg");

    // -6 등 정의되지 않은 음수 값 확인
    EXPECT_TRUE(EnumName(static_cast<ESparse>(-6)).IsEmpty());
}
