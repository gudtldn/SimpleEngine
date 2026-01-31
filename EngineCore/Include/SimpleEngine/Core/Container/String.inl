#pragma once
#include <algorithm>
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
namespace details
{
SE_CORE_API bool IsCharBoundary(StringView view, usize index);

/** 코드 포인트 개수 계산 */
SE_CORE_API usize CountCodePointsImpl(StringView view);

/** char32_t를 UTF-8 바이트 시퀀스로 인코딩 (최대 4바이트 + null) */
SE_CORE_API Array<char> EncodeCodePoint(char32 code_point);

/** 문자열의 마지막 코드 포인트를 디코딩하고 해당 바이트 길이를 반환 */
SE_CORE_API Optional<std::pair<char32, usize>> DecodeLastCodePoint(StringView view);

/** 대소문자 변환 */
SE_CORE_API String ToUpperImpl(StringView view, const char* locale);
SE_CORE_API String ToLowerImpl(StringView view, const char* locale);
}  // namespace details


template <typename Allocator>
BaseString<Allocator>::BaseString() noexcept
{
    data.Push('\0');
}

template <typename Allocator>
BaseString<Allocator>::BaseString(char32 code_point, SizeType repeat)
{
    if (repeat > 0)
    {
        if (Array<char> encoded_bytes = details::EncodeCodePoint(code_point); !encoded_bytes.IsEmpty())
        {
            Reserve(encoded_bytes.Len() * repeat);
            for (SizeType i = 0; i < repeat; ++i)
            {
                data.PushRange(encoded_bytes);
            }
        }
    }
    data.Push('\0');
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator=(char32 code_point)
{
    Clear();
    Push(code_point);
    return *this;
}

template <typename Allocator>
BaseString<Allocator>::BaseString(const char* literal)
    : BaseString(StringView{ literal })
{
}

template <typename Allocator>
BaseString<Allocator>::BaseString(const char* literal, SizeType length)
    : BaseString(StringView{ literal, length })
{
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator=(const char* literal)
{
    *this = StringView{ literal };
    return *this;
}

template <typename Allocator>
template <usize N>
BaseString<Allocator>::BaseString(const char (&literal)[N])
    : BaseString(StringView{ literal })
{
}

template <typename Allocator>
template <usize N>
BaseString<Allocator>& BaseString<Allocator>::operator=(const char (&literal)[N])
{
    *this = StringView{ literal };
    return *this;
}

template <typename Allocator>
BaseString<Allocator>::BaseString(StringView view)
{
    if (view.IsEmpty())
    {
        data.Push('\0');
        return;
    }

    data.Reserve(view.ByteLen());
    data.PushRange(view);
    data.Push('\0');
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator=(StringView view)
{
    if (Data() == view.Data())
    {
        return *this;
    }

    Clear();
    Append(view);

    return *this;
}

template <typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, char>
BaseString<Allocator>::BaseString(It first, Sent last)
{
    data.Push(first, last);
    data.Push('\0');
}

template <typename Allocator>
template <typename... Args>
BaseString<Allocator> BaseString<Allocator>::Format(std::format_string<Args...> fmt, Args&&... args)
{
    const std::string result_str = std::format(fmt, std::forward<Args>(args)...);
    return BaseString{ StringView{ result_str } };
}

template <typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, char>
BaseString<Allocator> BaseString<Allocator>::FromRange(Rng&& range)
{
    return BaseString{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename Allocator>
BaseString<Allocator>::SizeType BaseString<Allocator>::ByteLen() const noexcept
{
    return data.Len() > 0 ? data.Len() - 1 : 0;
}

template <typename Allocator>
BaseString<Allocator>::SizeType BaseString<Allocator>::CodePointLen() const
{
    return details::CountCodePointsImpl(StringView{ *this });
}

template <typename Allocator>
bool BaseString<Allocator>::IsEmpty() const noexcept
{
    return ByteLen() == 0;
}

template <typename Allocator>
BaseString<Allocator>::SizeType BaseString<Allocator>::Capacity() const noexcept
{
    return data.Capacity() > 0 ? data.Capacity() - 1 : 0;
}

template <typename Allocator>
void BaseString<Allocator>::Reserve(SizeType new_capacity)
{
    data.Reserve(new_capacity + 1); // null terminator 공간 추가
}

template <typename Allocator>
void BaseString<Allocator>::ResizeForOverwrite(SizeType new_size)
{
    data.ResizeUninitialized(new_size + 1); // null terminator 공간 추가
    data[new_size] = '\0';
}

template <typename Allocator>
void BaseString<Allocator>::ShrinkToFit()
{
    data.ShrinkToFit();
}

template <typename Allocator>
void BaseString<Allocator>::Push(char32 code_point)
{
    Array<char> encoded_bytes = details::EncodeCodePoint(code_point);

    data.Pop(); // null terminator 제거
    data.PushRange(encoded_bytes);
    data.Push('\0'); // null terminator 추가
}

template <typename Allocator>
Optional<char32> BaseString<Allocator>::Pop()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    Optional decode_result = details::DecodeLastCodePoint(StringView{ *this });
    SE_ASSERT(decode_result.HasValue(), "Failed to decode the last code point.");

    const auto& [code_point, byte_len] = decode_result.Value();
    Truncate(ByteLen() - byte_len);
    return code_point;
}

template <typename Allocator>
void BaseString<Allocator>::Append(const BaseString& other)
{
    Append(StringView{ other });
}

template <typename Allocator>
void BaseString<Allocator>::Append(const char* str)
{
    Append(StringView{ str });
}

template <typename Allocator>
void BaseString<Allocator>::Append(StringView view)
{
    if (view.IsEmpty())
    {
        return;
    }

    data.Pop();
    data.PushRange(view);
    data.Push('\0');
}

template <typename Allocator>
void BaseString<Allocator>::Insert(SizeType byte_idx, StringView view)
{
    SE_ASSERT(byte_idx <= ByteLen(), "Insert index out of bounds");
    SE_ASSERT(details::IsCharBoundary(StringView{ *this }, byte_idx), "Byte index is not a valid UTF-8 character boundary.");

    if (view.IsEmpty())
    {
        return;
    }

    data.Pop(); // null terminator 제거
    data.InsertRange(byte_idx, view);
    data.Push('\0'); // null terminator 다시 추가
}

template <typename Allocator>
void BaseString<Allocator>::RemoveRange(SizeType byte_idx, SizeType count)
{
    SE_ASSERT(byte_idx + count <= ByteLen(), "RemoveRange out of bounds");
    SE_ASSERT(details::IsCharBoundary(StringView{ *this }, byte_idx), "Byte index is not a valid UTF-8 character boundary.");
    SE_ASSERT(details::IsCharBoundary(StringView{ *this }, byte_idx + count), "Count is not a valid UTF-8 character boundary.");

    if (count == 0)
    {
        return;
    }

    data.RemoveRange(byte_idx, count);
}

template <typename Allocator>
void BaseString<Allocator>::Truncate(SizeType new_byte_len)
{
    if (new_byte_len >= ByteLen())
    {
        return;
    }
    SE_ASSERT(details::IsCharBoundary(Bytes(), new_byte_len), "Truncation position is not a valid UTF-8 character boundary.");

    data.Resize(new_byte_len + 1);
    *data.Back() = '\0';
}

template <typename Allocator>
void BaseString<Allocator>::Clear() noexcept
{
    data.Clear();
    data.Push('\0');
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::ToUpper(const char* locale) const
{
    return details::ToUpperImpl(StringView{ *this }, locale);
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::ToLower(const char* locale) const
{
    return details::ToLowerImpl(StringView{ *this }, locale);
}

template <typename Allocator>
bool BaseString<Allocator>::Contains(StringView view) const
{
    return StringView{ *this }.Contains(view);
}

template <typename Allocator>
bool BaseString<Allocator>::StartsWith(StringView view) const
{
    return StringView{ *this }.StartsWith(view);
}

template <typename Allocator>
bool BaseString<Allocator>::EndsWith(StringView view) const
{
    return StringView{ *this }.EndsWith(view);
}

template <typename Allocator>
Optional<typename BaseString<Allocator>::SizeType> BaseString<Allocator>::Find(StringView view, SizeType start_byte_pos) const
{
    return StringView{ *this }.Find(view, start_byte_pos);
}

template <typename Allocator>
Optional<typename BaseString<Allocator>::SizeType> BaseString<Allocator>::FindLast(StringView view, SizeType start_byte_pos) const
{
    return StringView{ *this }.FindLast(view, start_byte_pos);
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::Substring(SizeType start_index, SizeType byte_count) const
{
    return BaseString{ SubstringView(start_index, byte_count) };
}

template <typename Allocator>
StringView BaseString<Allocator>::SubstringView(SizeType start_index, SizeType byte_count) const
{
    SE_ASSERT(start_index <= ByteLen(), "Substring start index out of bounds");
    const SizeType max_len = ByteLen() - start_index;

    return StringView{
        Data() + start_index,
        std::min(byte_count, max_len)
    };
}

template <typename Allocator>
const char* BaseString<Allocator>::CStr() const noexcept
{
    return data.Data();
}

template <typename Allocator>
const char* BaseString<Allocator>::Data() const noexcept
{
    return data.Data();
}

template <typename Allocator>
char* BaseString<Allocator>::Data() noexcept
{
    return data.Data();
}

template <typename Allocator>
details::CodePointView BaseString<Allocator>::CodePoints() const
{
    return details::CodePointView{ Bytes() };
}

template <typename Allocator>
StringView BaseString<Allocator>::Bytes() const
{
    return StringView{ *this };
}

template <typename Allocator>
void BaseString<Allocator>::Swap(BaseString& other) noexcept
{
    std::swap(data, other.data);
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::operator+(const BaseString& other) const
{
    BaseString ret{ *this };
    ret.Append(other);
    return ret;
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::operator+(char32 code_point) const
{
    BaseString ret{ *this };
    ret.Push(code_point);
    return ret;
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::operator+(const char* str) const
{
    BaseString ret{ *this };
    ret.Append(StringView{ str });
    return ret;
}

template <typename Allocator>
BaseString<Allocator> BaseString<Allocator>::operator+(StringView view) const
{
    BaseString ret{ *this };
    ret.Append(view);
    return ret;
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator+=(const BaseString& other)
{
    Append(other);
    return *this;
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator+=(char32 code_point)
{
    Push(code_point);
    return *this;
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator+=(const char* str)
{
    Append(StringView{ str });
    return *this;
}

template <typename Allocator>
BaseString<Allocator>& BaseString<Allocator>::operator+=(StringView view)
{
    Append(view);
    return *this;
}

template <typename Allocator>

bool BaseString<Allocator>::operator==(const BaseString& other) const
{
    return StringView{ *this } == StringView{ other };
}

template <typename Allocator>

bool BaseString<Allocator>::operator==(const char* str) const
{
    return StringView{ *this } == StringView{ str };
}

template <typename Allocator>

bool BaseString<Allocator>::operator==(StringView view) const
{
    return StringView{ *this } == view;
}


template <typename Allocator>
std::strong_ordering BaseString<Allocator>::operator<=>(const BaseString& other) const
{
    return StringView{ *this } <=> StringView{ other };
}

template <typename Allocator>
std::strong_ordering BaseString<Allocator>::operator<=>(const char* other) const
{
    return StringView{ *this } <=> StringView{ other };
}

template <typename Allocator>
std::strong_ordering BaseString<Allocator>::operator<=>(StringView other) const
{
    return StringView{ *this } <=> other;
}
}  // namespace se
