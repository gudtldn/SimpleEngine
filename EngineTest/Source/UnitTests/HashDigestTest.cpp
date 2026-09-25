#include "gtest/gtest.h"

#include "SimpleEngine/Core/Types/HashDigest.h"

using namespace se;


TEST(HashDigestTest, TryFromHexRoundTrip)
{
    const u8 raw[32] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    };
    const ContentHash original = ContentHash::FromRaw(raw);
    const Optional<ContentHash> parsed = ContentHash::TryFromHex(original.ToHex());

    ASSERT_TRUE(parsed.HasValue());
    EXPECT_EQ(parsed.Value(), original);
}

TEST(HashDigestTest, TryFromHexAcceptsUppercaseHex)
{
    const Optional<ContentHash> lower = ContentHash::TryFromHex(String('a', ContentHash::DIGEST_SIZE * 2));
    const Optional<ContentHash> upper = ContentHash::TryFromHex(String('A', ContentHash::DIGEST_SIZE * 2));

    ASSERT_TRUE(lower.HasValue());
    ASSERT_TRUE(upper.HasValue());
    EXPECT_EQ(lower.Value(), upper.Value());
}

TEST(HashDigestTest, TryFromHexRejectsWrongLength)
{
    EXPECT_FALSE(ContentHash::TryFromHex("").HasValue());
    EXPECT_FALSE(ContentHash::TryFromHex("abcd").HasValue());
}

TEST(HashDigestTest, TryFromHexRejectsInvalidCharWithoutAsserting)
{
    // FromHex와 달리, 길이가 맞아도 잘못된 문자가 있으면 assert 없이 NullOpt를 반환해야 합니다.
    const String invalid_hex('!', ContentHash::DIGEST_SIZE * 2);
    EXPECT_FALSE(ContentHash::TryFromHex(invalid_hex).HasValue());
}

TEST(HashDigestTest, TryFromHexRejectsSingleInvalidCharAtEnd)
{
    String almost_valid('0', ContentHash::DIGEST_SIZE * 2);
    almost_valid.Data()[almost_valid.ByteLen() - 1] = 'z';

    EXPECT_FALSE(ContentHash::TryFromHex(almost_valid).HasValue());
}
