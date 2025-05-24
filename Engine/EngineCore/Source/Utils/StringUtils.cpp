module SimpleEngine.Utils;
import :StringUtils;

#include <assert.h>
#include <utf8.h>


namespace se::string_utils
{
std::u8string ToU8String(const std::string& in_str)
{
    std::u8string result;
    result.reserve(in_str.size());

    // 일단 그냥 assign해보고, 추후 문제가 있으면 utf8::replace_invalid 추가로 사용
    assert(utf8::is_valid(in_str.begin(), in_str.end()));

    result.assign(in_str.begin(), in_str.end());
    return result;
}

std::u8string ToU8String(const std::wstring& in_str)
{
    std::u8string result;
    result.reserve(in_str.size());

    if constexpr (sizeof(wchar_t) == 2)
    {
        utf8::utf16to8(in_str.begin(), in_str.end(), std::back_inserter(result));
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        utf8::utf32to8(in_str.begin(), in_str.end(), std::back_inserter(result));
    }
    else
    {
        // static_assert() // TODO: AlwaysFalse<T>만들고 static_assert 추가
    }
    return result;
}

std::u8string ToU8String(const std::u16string& in_str)
{
    std::u8string result;
    result.reserve(in_str.size());

    utf8::utf16to8(in_str.begin(), in_str.end(), std::back_inserter(result));
    return result;
}

std::u8string ToU8String(const std::u32string& in_str)
{
    std::u8string result;
    result.reserve(in_str.size());

    utf8::utf32to8(in_str.begin(), in_str.end(), std::back_inserter(result));
    return result;
}
}
