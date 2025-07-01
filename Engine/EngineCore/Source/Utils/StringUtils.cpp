module;
#include <unicode/unistr.h>
module SimpleEngine.Utils;
import :StringUtils;

import SimpleEngine.Platform.Types;
import SimpleEngine.Core.TypeTraits;
import <cassert>;


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
    const icu::UnicodeString ustr{ in_str.data(), static_cast<int32>(in_str.size()) };

    std::string result;
    ustr.toUTF8String(result);

    return ToU8String(result);
}

std::u8string ToU8String(std::u16string_view in_str)
{
    const icu::UnicodeString ustr{ in_str.data(), static_cast<int32>(in_str.size()) };

    std::string result;
    ustr.toUTF8String(result);

    return ToU8String(result);
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

    const icu::UnicodeString ustr = icu::UnicodeString::fromUTF32(
        reinterpret_cast<const UChar32*>(in_str.data()),
        static_cast<int32_t>(in_str.length())
    );

    std::string result;
    ustr.toUTF8String(result);

    return ToU8String(result);
}
}
