#include "SimpleEngine/Utility/StringUtils.h"


namespace
{
// =============================================================================
// UTF 인코딩 변환 헬퍼 (RFC 3629)
// =============================================================================

/**
 * 유니코드 코드 포인트를 UTF-8 바이트 시퀀스로 인코딩하여 출력 문자열에 추가합니다.
 * @param cp 인코딩할 유니코드 코드 포인트
 * @param out UTF-8 바이트가 추가될 대상 문자열
 */
void AppendCodePointAsUtf8(u32 cp, std::string& out)
{
    if (cp < 0x80u)
    {
        out += static_cast<char>(cp);
    }
    else if (cp < 0x800u)
    {
        out += static_cast<char>(0xC0u | (cp >> 6));
        out += static_cast<char>(0x80u | (cp & 0x3Fu));
    }
    else if (cp < 0x10000u)
    {
        out += static_cast<char>(0xE0u | (cp >> 12));
        out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        out += static_cast<char>(0x80u | (cp & 0x3Fu));
    }
    else
    {
        out += static_cast<char>(0xF0u | (cp >> 18));
        out += static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
        out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        out += static_cast<char>(0x80u | (cp & 0x3Fu));
    }
}

/**
 * UTF-16 시퀀스를 UTF-8로 변환합니다. (Surrogate 쌍 처리 포함)
 * @param data 변환할 UTF-16 문자열 배열의 포인터
 * @param len 문자열의 길이
 * @return UTF-8로 변환된 se::String 객체
 */
se::String Utf16ToUtf8(const char16_t* data, usize len)
{
    std::string out;
    out.reserve(len); // 최소 예상 크기 확보

    for (usize i = 0; i < len; ++i)
    {
        const u32 unit = static_cast<u32>(data[i]);
        if (unit >= 0xD800u && unit <= 0xDBFFu)
        {
            // High Surrogate: 이어지는 Low Surrogate와 결합하여 처리
            if (i + 1 < len)
            {
                const u32 low = static_cast<u32>(data[i + 1]);
                if (low >= 0xDC00u && low <= 0xDFFFu)
                {
                    const u32 cp = 0x10000u + ((unit - 0xD800u) << 10) + (low - 0xDC00u);
                    AppendCodePointAsUtf8(cp, out);
                    ++i;
                    continue;
                }
            }
            // 단독 High Surrogate: U+FFFD(대체 문자)로 치환
            AppendCodePointAsUtf8(0xFFFDu, out);
        }
        else if (unit >= 0xDC00u && unit <= 0xDFFFu)
        {
            // 단독 Low Surrogate: U+FFFD(대체 문자)로 치환
            AppendCodePointAsUtf8(0xFFFDu, out);
        }
        else
        {
            AppendCodePointAsUtf8(unit, out);
        }
    }

    return se::String{ out.data(), out.size() };
}

/**
 * UTF-32 시퀀스를 UTF-8로 변환합니다.
 * @param data 변환할 UTF-32 문자열 배열의 포인터
 * @param len 문자열의 길이
 * @return UTF-8로 변환된 se::String 객체
 */
se::String Utf32ToUtf8(const char32_t* data, usize len)
{
    std::string out;
    out.reserve(len); // 최소 예상 크기 확보

    for (usize i = 0; i < len; ++i)
    {
        AppendCodePointAsUtf8(static_cast<u32>(data[i]), out);
    }

    return se::String{ out.data(), out.size() };
}
} // namespace

namespace se
{
String StringUtils::ToString(std::string_view in_str)
{
    return String{ in_str.data(), in_str.size() };
}

String StringUtils::ToString(std::wstring_view in_str)
{
#if SE_PLATFORM_WINDOWS
    // Windows: wchar_t는 16비트(UTF-16)
    static_assert(sizeof(wchar_t) == sizeof(char16_t));
    return Utf16ToUtf8(reinterpret_cast<const char16_t*>(in_str.data()), in_str.size());
#else
    // Linux/macOS: wchar_t는 32비트(UTF-32)
    static_assert(sizeof(wchar_t) == sizeof(char32_t));
    return Utf32ToUtf8(reinterpret_cast<const char32_t*>(in_str.data()), in_str.size());
#endif
}

String StringUtils::ToString(std::u8string_view in_str)
{
    return String{ reinterpret_cast<const char*>(in_str.data()), in_str.size() };
}

String StringUtils::ToString(std::u16string_view in_str)
{
    return Utf16ToUtf8(in_str.data(), in_str.size());
}

String StringUtils::ToString(std::u32string_view in_str)
{
    return Utf32ToUtf8(in_str.data(), in_str.size());
}
} // namespace se
