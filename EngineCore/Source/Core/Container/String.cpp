#include "Core/Container/String.h"
#include "Core/Container/StringView.h"
#include "Utility/Debug.h"

#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/utf8.h>


namespace se::detail
{
bool IsCharBoundary(StringView view, usize index)
{
    // 문자열의 시작과 끝은 항상 유효한 경계
    if (index == 0 || index == view.ByteLen())
    {
        return true;
    }

    // 인덱스가 범위를 벗어나면 유효하지 않은 경계
    if (index > view.ByteLen())
    {
        return false;
    }

    // 해당 바이트가 연속 바이트(10xxxxxx)가 아니면 유효한 경계
    return U8_IS_LEAD(view[index]) || U8_IS_SINGLE(view[index]);
}

usize CountCodePointsImpl(StringView view)
{
    if (view.IsEmpty())
    {
        return 0;
    }
    usize count = 0;
    const char* s = view.Data();
    const auto length = static_cast<int32>(view.ByteLen());
    for (int32 i = 0; i < length;)
    {
        UChar32 c;
        U8_NEXT(s, i, length, c); // NOLINT(*-assignment-in-if-condition, *-inc-dec-in-conditions)
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

Optional<std::pair<char32, usize>> DecodeLastCodePoint(StringView view)
{
    if (view.IsEmpty())
    {
        return std::nullopt;
    }

    const char* s = view.Data();
    int32 i = static_cast<int32>(view.ByteLen());
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

String ToUpperImpl(StringView view, const char* locale)
{
    if (view.IsEmpty())
    {
        return {};
    }

    icu::UnicodeString ustr{
        view.Data(),
        static_cast<int32>(view.ByteLen()),
        "UTF-8"
    };
    ustr.toUpper(icu::Locale{ locale });

    std::string result_str;
    ustr.toUTF8String(result_str);
    return { StringView{ result_str } };
}

String ToLowerImpl(StringView view, const char* locale)
{
    if (view.IsEmpty())
    {
        return {};
    }

    icu::UnicodeString ustr{
        view.Data(),
        static_cast<int32>(view.ByteLen()),
        "UTF-8"
    };
    ustr.toLower(icu::Locale{ locale });

    std::string result_str;
    ustr.toUTF8String(result_str);
    return { StringView{ result_str } };
}

CodePointIterator::reference CodePointIterator::operator*() const
{
    SE_ASSERT(ptr != nullptr, "Dereferencing null iterator");

    UChar32 c;
    constexpr int32 offset = 0;

    // U8_GET_UNSAFE(s, i, c)
    // s: 현재 위치 포인터 (ptr이 문자열 시작처럼 동작)
    // i: 오프셋 (현재 위치를 읽으므로 0)
    // c: 결과 코드 포인트
    U8_GET_UNSAFE(ptr, offset, c);
    return static_cast<char32>(c);
}

CodePointIterator& CodePointIterator::operator++()
{
    SE_ASSERT(ptr != nullptr, "Incrementing null iterator");

    int32 offset = 0;
    UChar32 dummy_c;

    // U8_NEXT_UNSAFE(s, i, c)
    // s: 현재 위치 포인터
    // i: 오프셋
    // c: 결과 코드 포인트

    // U8_NEXT_UNSAFE를 사용하여 코드 포인트를 읽고, offset을 코드 포인트 길이로 업데이트
    U8_NEXT_UNSAFE(ptr, offset, dummy_c);

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
}  // namespace se::detail
