#include "Core/Container/String.h"

#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>


namespace se::details
{
bool IsCharBoundary(std::string_view view, usize index)
{
    // 문자열의 시작과 끝은 항상 유효한 경계
    if (index == 0 || index == view.length())
    {
        return true;
    }

    // 인덱스가 범위를 벗어나면 유효하지 않은 경계
    if (index > view.length())
    {
        return false;
    }

    // 해당 바이트가 연속 바이트(10xxxxxx)가 아니면 유효한 경계
    return U8_IS_LEAD(view[index]) || U8_IS_SINGLE(view[index]);
}

usize CountCodePointsImpl(std::string_view view)
{
    if (view.empty())
    {
        return 0;
    }
    usize count = 0;
    const char* s = view.data();
    const auto length = static_cast<int32>(view.length());
    for (int32 i = 0; i < length;)
    {
        UChar32 c;
        U8_NEXT(s, i, length, c);
        ++count;
    }
    return count;
}

Array<char> EncodeCodePoint(char32 code_point)
{
    Array<char> out_bytes;
    out_bytes.Resize(U8_MAX_LENGTH);
    int32_t length = 0;
    UBool is_error = false;

    U8_APPEND(out_bytes.Data(), length, U8_MAX_LENGTH, code_point, is_error);

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

Optional<std::pair<char32, usize>> DecodeLastCodePoint(std::string_view view)
{
    if (view.empty())
    {
        return std::nullopt;
    }

    const char* s = view.data();
    int32 i = static_cast<int32>(view.length());
    const int32 prev_i = i;

    // i를 이전 코드 포인트의 시작 위치로 이동시킴
    UChar32 c;
    U8_PREV(s, 0, i, c);

    if (U_IS_SURROGATE(c))
    {
        return std::nullopt;
    }

    auto byte_len = static_cast<String::SizeType>(prev_i - i);
    return std::make_pair(static_cast<char32>(c), byte_len);
}

String ToUpperImpl(std::string_view view, const char* locale)
{
    if (view.empty())
    {
        return {};
    }

    icu::UnicodeString ustr{
        view.data(),
        static_cast<int32>(view.length()),
        "UTF-8"
    };
    ustr.toUpper(icu::Locale{ locale });

    std::string result_str;
    ustr.toUTF8String(result_str);
    return { result_str };
}

String ToLowerImpl(std::string_view view, const char* locale)
{
    if (view.empty())
    {
        return {};
    }

    icu::UnicodeString ustr{
        view.data(),
        static_cast<int32>(view.length()),
        "UTF-8"
    };
    ustr.toLower(icu::Locale{ locale });

    std::string result_str;
    ustr.toUTF8String(result_str);
    return { result_str };
}

CodePointIterator::reference CodePointIterator::operator*() const
{
    assert(ptr != nullptr && "Dereferencing null iterator");

    UChar32 c;
    constexpr int32 offset = 0;

    // U8_GET_UNSAFE(s, i, c)
    // s: 현재 위치 포인터 (ptr이 문자열 시작처럼 동작)
    // i: 오프셋 (현재 위치를 읽으므로 0)
    // c: 결과 코드 포인트
    U8_GET_UNSAFE(reinterpret_cast<const uint8*>(ptr), offset, c);
    return static_cast<char32>(c);
}

CodePointIterator& CodePointIterator::operator++()
{
    assert(ptr != nullptr && "Incrementing null iterator");

    int32 offset = 0;
    UChar32 dummy_c;

    // U8_NEXT_UNSAFE를 사용하여 코드 포인트를 읽고, offset을 코드 포인트 길이로 업데이트
    U8_NEXT_UNSAFE(reinterpret_cast<const uint8*>(ptr), offset, dummy_c);

    // 업데이트된 offset(코드 포인트의 바이트 길이)만큼 ptr을 전진
    ptr += offset;

    return *this;
}

CodePointIterator CodePointIterator::operator++(int)
{
    const CodePointIterator temp = *this;
    ++(*this);
    return temp;
}
}
