#include "gtest/gtest.h"

#include "SimpleEngine/Core/Types/Guid.h"

using namespace se;


TEST(GuidTest, TryFromStringParsesKnownValue)
{
    const StringView text = "01234567-89ab-cdef-0123-456789abcdef";
    const Optional<Guid> parsed = Guid::TryFromString(text);

    ASSERT_TRUE(parsed.HasValue());
    EXPECT_EQ(parsed.Value().ToString(), text);
}

TEST(GuidTest, TryFromStringRoundTripsNewGuid)
{
    const Guid original = Guid::NewGuid();
    const Optional<Guid> parsed = Guid::TryFromString(original.ToString());

    ASSERT_TRUE(parsed.HasValue());
    EXPECT_EQ(parsed.Value(), original);
}

TEST(GuidTest, TryFromStringAcceptsUppercaseHex)
{
    const Optional<Guid> lower = Guid::TryFromString("01234567-89ab-cdef-0123-456789abcdef");
    const Optional<Guid> upper = Guid::TryFromString("01234567-89AB-CDEF-0123-456789ABCDEF");

    ASSERT_TRUE(lower.HasValue());
    ASSERT_TRUE(upper.HasValue());
    EXPECT_EQ(lower.Value(), upper.Value());
}

TEST(GuidTest, TryFromStringRejectsWrongLength)
{
    EXPECT_FALSE(Guid::TryFromString("").HasValue());
    EXPECT_FALSE(Guid::TryFromString("too-short").HasValue());
}

TEST(GuidTest, TryFromStringRejectsMissingDashes)
{
    // 길이(36)는 같지만 대시가 있어야 할 자리에 다른 문자가 있는 경우입니다.
    const String no_dashes('0', 36);
    EXPECT_FALSE(Guid::TryFromString(no_dashes).HasValue());
}

TEST(GuidTest, TryFromStringRejectsInvalidHexChar)
{
    EXPECT_FALSE(Guid::TryFromString("zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz").HasValue());
}

TEST(GuidTest, TryFromStringDistinguishesFailureFromNone)
{
    // 전부 0인 GUID는 정상 값(None)으로 읽히고, 형식이 틀린 문자열만 실패해야 합니다.
    const Optional<Guid> none = Guid::TryFromString("00000000-0000-0000-0000-000000000000");
    ASSERT_TRUE(none.HasValue());
    EXPECT_EQ(none.Value(), Guid::None);
    EXPECT_FALSE(Guid::TryFromString("invalid").HasValue());
}

TEST(GuidTest, FromStringParsesLiteralAtCompileTime)
{
    // 형식이 틀린 상수는 컴파일 에러가 나므로, 올바른 상수가 상수식에서 파싱되는지만 확인합니다.
    constexpr Guid parsed = Guid::FromString("01234567-89ab-cdef-0123-456789abcdef");
    EXPECT_EQ(parsed.ToString(), "01234567-89ab-cdef-0123-456789abcdef");
}
