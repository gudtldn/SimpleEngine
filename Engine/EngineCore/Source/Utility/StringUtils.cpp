module;
#include <unicode/unistr.h>
#include <unicode/locid.h>
module SimpleEngine.Utility;
import :StringUtils;

import SimpleEngine.Types;
import <cassert>;

using namespace icu;


namespace
{
std::u8string ToU8String(const UnicodeString& in_ustr)
{
    std::string result;
    in_ustr.toUTF8String(result);
    return se::string_utils::ToU8String(result);
}
}

namespace se::string_utils
{
std::u8string ToU8String(std::string_view in_str)
{
    return std::u8string{
        reinterpret_cast<const char8_t*>(in_str.data()),
        in_str.size()
    };
}

std::u8string ToU8String(std::wstring_view in_str)
{
    const UnicodeString ustr{
        in_str.data(),
        static_cast<int32>(in_str.size())
    };
    return ::ToU8String(ustr);
}

std::u8string ToU8String(std::u16string_view in_str)
{
    const UnicodeString ustr{
        in_str.data(),
        static_cast<int32>(in_str.size())
    };
    return ::ToU8String(ustr);
}

std::u8string ToU8String(std::u32string_view in_str)
{
    static_assert(
        sizeof(std::u32string_view::value_type) == sizeof(UChar32),
        "char32_t and UChar32 have different sizes."
    );
    static_assert(
        alignof(std::u32string_view::value_type) == alignof(UChar32),
        "char32_t and UChar32 have different alignments."
    );

    const UnicodeString ustr = UnicodeString::fromUTF32(
        reinterpret_cast<const UChar32*>(in_str.data()),
        static_cast<int32_t>(in_str.size())
    );
    return ::ToU8String(ustr);
}

std::u8string ToU8UpperCase(std::u8string_view in_str)
{
    UnicodeString ustr = UnicodeString::fromUTF8(in_str);
    ustr.toUpper();
    return ::ToU8String(ustr);
}

std::u8string ToU8LowerCase(std::u8string_view in_str)
{
    UnicodeString ustr = UnicodeString::fromUTF8(in_str);
    ustr.toLower();
    return ::ToU8String(ustr);
}
}
