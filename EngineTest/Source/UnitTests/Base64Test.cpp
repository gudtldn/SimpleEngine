#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Utility/Base64.h"

#include <ranges>
#include <string_view>

using namespace se;


namespace
{
/** 문자열의 바이트를 그대로 담은 배열을 만듭니다. */
[[nodiscard]] Array<u8> BytesOf(std::string_view text)
{
    return Array<u8>::FromRange(text | std::views::transform([](char c) { return static_cast<u8>(c); }));
}
} // namespace


TEST(Base64Test, EncodesRfc4648Vectors)
{
    EXPECT_EQ(base64::Encode(BytesOf("")), "");
    EXPECT_EQ(base64::Encode(BytesOf("f")), "Zg==");
    EXPECT_EQ(base64::Encode(BytesOf("fo")), "Zm8=");
    EXPECT_EQ(base64::Encode(BytesOf("foo")), "Zm9v");
    EXPECT_EQ(base64::Encode(BytesOf("foob")), "Zm9vYg==");
    EXPECT_EQ(base64::Encode(BytesOf("fooba")), "Zm9vYmE=");
    EXPECT_EQ(base64::Encode(BytesOf("foobar")), "Zm9vYmFy");
}

TEST(Base64Test, DecodesRfc4648Vectors)
{
    const std::string_view decoded[] = { "", "f", "fo", "foo", "foob", "fooba", "foobar" };
    const std::string_view encoded[] = { "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy" };

    for (const auto [expected, text] : std::views::zip(decoded, encoded))
    {
        const auto bytes = base64::Decode(text);
        ASSERT_TRUE(bytes.HasValue()) << text;
        EXPECT_EQ(*bytes, BytesOf(expected)) << text;
    }
}

TEST(Base64Test, AllByteValuesRoundTrip)
{
    Array<u8> bytes;
    for (usize value = 0; value < 256; ++value)
    {
        bytes.Push(static_cast<u8>(value));
    }

    // 0xFB 0xFF는 '+'와 '/'를 모두 씀
    const String text = base64::Encode(bytes);
    EXPECT_TRUE(text.Contains("+"));
    EXPECT_TRUE(text.Contains("/"));

    const auto decoded = base64::Decode(text);
    ASSERT_TRUE(decoded.HasValue());
    EXPECT_EQ(*decoded, bytes);
}

TEST(Base64Test, DecodeRejectsInvalidText)
{
    const std::string_view cases[] = {
        "Zm9",      // 길이가 4의 배수가 아님
        "Zg=",
        "Zm9$",     // 표준 알파벳이 아닌 문자
        "Zm-v",     // URL 안전 알파벳
        " Zg=",     // 공백
        "Zg==Zm9v", // 끝이 아닌 곳의 패딩
        "Z===",     // 패딩이 세 글자
        "====",
        "Zh==",     // 남는 비트가 0이 아님 (정규형은 "Zg==")
        "Zm9=",     // 남는 비트가 0이 아님 (정규형은 "Zm8=")
    };

    for (const std::string_view text : cases)
    {
        EXPECT_FALSE(base64::Decode(text).HasValue()) << text;
    }
}
