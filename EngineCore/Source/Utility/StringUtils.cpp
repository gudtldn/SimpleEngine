#include "Utility/StringUtils.h"

#include <unicode/unistr.h>
#include <unicode/locid.h>

using namespace icu;


namespace
{
se::String ToString(const UnicodeString& in_ustr)
{
    std::string result;
    in_ustr.toUTF8String(result);
    return se::String{ result.c_str(), result.size() };
}
}

namespace se::utility
{
String ToString(std::string_view in_str)
{
    const UnicodeString ustr = UnicodeString::fromUTF8(in_str);
    return ::ToString(ustr);
}

String ToString(std::wstring_view in_str)
{
#if SE_PLATFORM_WINDOWS
    // Windows: wchar_t는 16비트(UTF-16)
    const UnicodeString ustr{
        reinterpret_cast<const UChar*>(in_str.data()),
        static_cast<int32>(in_str.length())
    };
#else
    // Linux/macOS: wchar_t는 32비트(UTF-32)
    const UnicodeString ustr = UnicodeString::fromUTF32(
        reinterpret_cast<const UChar32*>(in_str.data()),
        static_cast<int32>(in_str.length())
    );
#endif
    return ::ToString(ustr);
}

String ToString(std::u8string_view in_str)
{
    const UnicodeString ustr = UnicodeString::fromUTF8(in_str);
    return ::ToString(ustr);
}

String ToString(std::u16string_view in_str)
{
    const UnicodeString ustr{
        in_str.data(),
        static_cast<int32>(in_str.size())
    };
    return ::ToString(ustr);
}

String ToString(std::u32string_view in_str)
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
    return ::ToString(ustr);
}
}
