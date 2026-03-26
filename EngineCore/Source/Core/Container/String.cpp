// ReSharper disable CppDFAConstantParameter

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Utility/Debug.h"

#include <icu4x/CaseMapper.hpp>
#include <icu4x/Locale.hpp>

#include <string>


namespace
{
// =============================================================================
// UTF-8 헬퍼 (RFC 3629)
//
// UTF-8 인코딩 규칙:
//   1바이트: 0xxxxxxx                            -> U+0000 ~ U+007F
//   2바이트: 110xxxxx 10xxxxxx                   -> U+0080 ~ U+07FF
//   3바이트: 1110xxxx 10xxxxxx 10xxxxxx          -> U+0800 ~ U+FFFF
//   4바이트: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> U+10000 ~ U+10FFFF
//   연속 바이트: 10xxxxxx
// =============================================================================

constexpr int32 UTF8_MAX_LENGTH = 4;

/** 단일 ASCII 바이트인지 확인합니다. (0xxxxxxx) */
constexpr bool Utf8IsSingle(char c)
{
    return (static_cast<uint8>(c) & 0x80u) == 0;
}

/** 멀티바이트 시퀀스의 선행 바이트인지 확인합니다. (11xxxxxx) */
constexpr bool Utf8IsLead(char c)
{
    return (static_cast<uint8>(c) & 0xC0u) == 0xC0u;
}

/** 연속 바이트인지 확인합니다. (10xxxxxx) */
constexpr bool Utf8IsTrail(char c)
{
    return (static_cast<uint8>(c) & 0xC0u) == 0x80u;
}

/** 선행 바이트로부터 시퀀스 길이를 반환합니다. 유효하지 않으면 1을 반환합니다. */
constexpr int32 Utf8SequenceLen(char lead)
{
    const uint8 b = static_cast<uint8>(lead);
    if ((b & 0x80u) == 0)     { return 1; } // 0xxxxxxx
    if ((b & 0xE0u) == 0xC0u) { return 2; } // 110xxxxx
    if ((b & 0xF0u) == 0xE0u) { return 3; } // 1110xxxx
    if ((b & 0xF8u) == 0xF0u) { return 4; } // 11110xxx
    return 1; // 잘못된 바이트인 경우 1바이트씩 건너뜀
}

/** 코드 포인트가 UTF-16 서로게이트 범위인지 확인합니다. (U+D800 ~ U+DFFF) */
constexpr bool IsUtf8Surrogate(char32_t c)
{
    return (static_cast<uint32>(c) & 0xFFFFF800u) == 0xD800u;
}

/**
 * 경계 검사 없이 코드 포인트를 디코딩합니다. (i 변경 없음)
 * @warning 호출자가 ptr[i]에서 시작하는 유효한 UTF-8 시퀀스를 보장해야 합니다.
 */
constexpr char32_t Utf8GetUnsafe(const char* s, int32 i)
{
    const uint8 b0 = static_cast<uint8>(s[i]);
    if ((b0 & 0x80u) == 0)
    {
        return static_cast<char32_t>(b0);
    }
    if ((b0 & 0xE0u) == 0xC0u)
    {
        return static_cast<char32_t>(((b0 & 0x1Fu) << 6) | (static_cast<uint8>(s[i + 1]) & 0x3Fu));
    }
    if ((b0 & 0xF0u) == 0xE0u)
    {
        return static_cast<char32_t>(((b0 & 0x0Fu) << 12) | ((static_cast<uint8>(s[i + 1]) & 0x3Fu) << 6) | (static_cast<uint8>(s[i + 2]) & 0x3Fu));
    }
    return static_cast<char32_t>(((b0 & 0x07u) << 18)
        | ((static_cast<uint8>(s[i + 1]) & 0x3Fu) << 12)
        | ((static_cast<uint8>(s[i + 2]) & 0x3Fu) << 6)
        | (static_cast<uint8>(s[i + 3]) & 0x3Fu));
}

/**
 * 경계 검사 없이 코드 포인트를 읽고 i를 다음 위치로 전진합니다.
 * @warning 호출자가 유효한 UTF-8 시퀀스를 보장해야 합니다.
 */
void Utf8NextUnsafe(const char* s, int32& i, char32_t& out_c)
{
    out_c = Utf8GetUnsafe(s, i);
    i += Utf8SequenceLen(s[i]);
}

/** 경계 검사 포함 코드 포인트 읽기. 잘못된 시퀀스는 U+FFFD로 치환합니다. */
void Utf8Next(const char* s, int32& i, int32 length, char32_t& out_c)
{
    const uint8 b0 = static_cast<uint8>(s[i]);
    const int32 seq_len = Utf8SequenceLen(static_cast<char>(b0));

    // 버퍼 끝을 초과했는지?
    if (i + seq_len > length)
    {
        out_c = 0xFFFD;
        ++i;
        return;
    }

    // 연속 바이트 유효성을 검사
    for (int32 k = 1; k < seq_len; ++k)
    {
        if (!Utf8IsTrail(s[i + k]))
        {
            out_c = 0xFFFD;
            ++i;
            return;
        }
    }

    out_c = Utf8GetUnsafe(s, i);
    i += seq_len;
}

/** 역방향으로 코드 포인트를 읽고 i를 해당 시퀀스의 시작으로 이동합니다. */
void Utf8Prev(const char* s, int32 start, int32& i, char32_t& out_c)
{
    --i;
    // 연속 바이트(10xxxxxx)는 Skip
    while (i > start && Utf8IsTrail(s[i]))
    {
        --i;
    }
    out_c = Utf8GetUnsafe(s, i);
}

/**
 * 코드 포인트를 UTF-8 바이트로 인코딩하여 buf[offset]부터 씁니다.
 * capacity를 초과하면 out_error를 true로 설정합니다.
 */
void Utf8Append(char* buf, int32& offset, int32 capacity, char32_t c, bool& out_error)
{
    const uint32 cp = static_cast<uint32>(c);

    if (cp < 0x80u)
    {
        if (offset + 1 > capacity) { out_error = true; return; }
        buf[offset++] = static_cast<char>(cp);
    }
    else if (cp < 0x800u)
    {
        if (offset + 2 > capacity) { out_error = true; return; }
        buf[offset++] = static_cast<char>(0xC0u | (cp >> 6));
        buf[offset++] = static_cast<char>(0x80u | (cp & 0x3Fu));
    }
    else if (cp < 0x10000u)
    {
        if (offset + 3 > capacity) { out_error = true; return; }
        buf[offset++] = static_cast<char>(0xE0u | (cp >> 12));
        buf[offset++] = static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        buf[offset++] = static_cast<char>(0x80u | (cp & 0x3Fu));
    }
    else
    {
        if (offset + 4 > capacity) { out_error = true; return; }
        buf[offset++] = static_cast<char>(0xF0u | (cp >> 18));
        buf[offset++] = static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu));
        buf[offset++] = static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
        buf[offset++] = static_cast<char>(0x80u | (cp & 0x3Fu));
    }
}
/**
 * locale 문자열을 파싱하여 ICU4X Locale 객체를 반환합니다.
 * @note ICU4C 형식("tr_TR")을 BCP47 형식("tr-TR")으로 자동 변환합니다.
 *       Debug에서 파싱 실패 시 assert, Release에서는 "und"로 폴백합니다.
 */
std::unique_ptr<icu4x::Locale> ParseLocale(const char* locale)
{
    if (!locale || !locale[0])
    {
        return icu4x::Locale::from_string("und").ok().value();
    }

    // ICU4C 형식("tr_TR") -> BCP47 형식("tr-TR")으로 변환
    std::string normalized{ locale };
    for (char& c : normalized)
    {
        if (c == '_') { c = '-'; }
    }

    auto result = icu4x::Locale::from_string(normalized);
    SE_ASSERT(result.is_ok(), "Invalid locale string: {}", locale);
    if (result.is_ok())
    {
        return std::move(result).ok().value();
    }
    return icu4x::Locale::from_string("und").ok().value();
}
} // namespace


namespace se::detail
{
bool IsCharBoundary(StringView view, usize index)
{
    // 문자열의 시작과 끝은 항상 유효한 경계이어야 함
    if (index == 0 || index == view.ByteLen())
    {
        return true;
    }

    // 만약 인덱스가 범위를 벗어나면 유효하지 않은 경계
    if (index > view.ByteLen())
    {
        return false;
    }

    // 연속 바이트(10xxxxxx)가 아니면 유효한 경계
    return Utf8IsLead(view[index]) || Utf8IsSingle(view[index]);
}

usize CountCodePointsImpl(StringView view)
{
    if (view.IsEmpty())
    {
        return 0;
    }
    usize count = 0;
    const char* s = view.Data();
    const int32 length = static_cast<int32>(view.ByteLen());
    for (int32 i = 0; i < length;)
    {
        char32_t c;
        Utf8Next(s, i, length, c);
        ++count;
    }
    return count;
}

Array<char> EncodeCodePoint(char32_t code_point)
{
    Array<char> out_bytes;
    out_bytes.Resize(UTF8_MAX_LENGTH);
    int32 length = 0;
    bool is_error = false;

    Utf8Append(out_bytes.Data(), length, UTF8_MAX_LENGTH, code_point, is_error);

    if (!is_error)
    {
        out_bytes.Resize(length);
    }
    else
    {
        out_bytes.Clear();
    }
    return out_bytes;
}

Optional<std::pair<char32_t, usize>> DecodeLastCodePoint(StringView view)
{
    if (view.IsEmpty())
    {
        return NullOpt;
    }

    const char* s = view.Data();
    int32 i = static_cast<int32>(view.ByteLen());
    const int32 prev_i = i;

    char32_t c;
    Utf8Prev(s, 0, i, c);

    if (IsUtf8Surrogate(c))
    {
        return NullOpt;
    }

    const usize byte_len = static_cast<String::SizeType>(prev_i - i);
    return std::make_pair(c, byte_len);
}

String ToUpperImpl(StringView view, const char* locale)
{
    if (view.IsEmpty())
    {
        return {};
    }

    const std::unique_ptr<icu4x::Locale> locale_obj = ParseLocale(locale);
    auto result = icu4x::CaseMapper::uppercase_with_compiled_data(std::string_view{ view }, *locale_obj);
    if (!result.is_ok())
    {
        return String{ view };
    }
    const std::string s = std::move(result).ok().value();
    return String{ s.data(), s.size() };
}

String ToLowerImpl(StringView view, const char* locale)
{
    if (view.IsEmpty())
    {
        return {};
    }

    const std::unique_ptr<icu4x::Locale> locale_obj = ParseLocale(locale);
    auto result = icu4x::CaseMapper::lowercase_with_compiled_data(std::string_view{ view }, *locale_obj);
    if (!result.is_ok())
    {
        return String{ view };
    }
    const std::string s = std::move(result).ok().value();
    return String{ s.data(), s.size() };
}

CodePointIterator::reference CodePointIterator::operator*() const
{
    SE_ASSERT(ptr != nullptr, "Dereferencing null iterator");

    constexpr int32 offset = 0;
    return Utf8GetUnsafe(reinterpret_cast<const char*>(ptr), offset);
}

CodePointIterator& CodePointIterator::operator++()
{
    SE_ASSERT(ptr != nullptr, "Incrementing null iterator");

    int32 offset = 0;
    char32_t dummy_c;
    Utf8NextUnsafe(reinterpret_cast<const char*>(ptr), offset, dummy_c);
    ptr += offset;

    return *this;
}

CodePointIterator CodePointIterator::operator++(int)
{
    const CodePointIterator temp = *this;
    ++(*this);
    return temp;
}
} // namespace se::detail
