#include "doctest/doctest.h"

#include "SimpleEngine/Utility/StringUtils.h"


TEST_SUITE("SimpleEngine.StringUtils")
{
using namespace se::utility::string;

static const se::u8string ValidateString = u8"Hello World 안녕 こんちは 👍";


TEST_CASE("ToU8String")
{
    SUBCASE("std::string_view")
    {
        const se::string str = "Hello World 안녕 こんちは 👍";
        const se::u8string u8str = ToU8String(str);
        CHECK(u8str == ValidateString);
    }

    SUBCASE("std::wstring_view")
    {
        const se::wstring wstr = L"Hello World 안녕 こんちは 👍";
        const se::u8string u8str = ToU8String(wstr);
        CHECK(u8str == ValidateString);
    }

    SUBCASE("std::u16string_view")
    {
        const se::u16string u16str = u"Hello World 안녕 こんちは 👍";
        const se::u8string u8str = ToU8String(u16str);
        CHECK(u8str == ValidateString);
    }

    SUBCASE("std::u32string_view")
    {
        const se::u32string u32str = U"Hello World 안녕 こんちは 👍";
        const se::u8string u8str = ToU8String(u32str);
        CHECK(u8str == ValidateString);
    }
}

TEST_CASE("ToU8UpperCase")
{
    const se::u8string test_case = ValidateString;
    const se::u8string validate = u8"HELLO WORLD 안녕 こんちは 👍";

    const se::u8string result = ToU8UpperCase(validate);
    CHECK(result == validate);
}

TEST_CASE("ToU8LowerCase")
{
    const se::u8string test_case = ValidateString;
    const se::u8string validate = u8"hello world 안녕 こんちは 👍";

    const se::u8string result = ToU8LowerCase(validate);
    CHECK(result == validate);
}
}
