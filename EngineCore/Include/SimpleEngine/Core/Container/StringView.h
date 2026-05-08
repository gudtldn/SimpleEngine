#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <algorithm>
#include <compare>
#include <format>
#include <string_view>


namespace se
{
// Forward declarations
template <typename Allocator>
class BaseString;

template <typename T>
class DefaultAllocator;

using String = BaseString<DefaultAllocator<char>>;


/**
 * 문자열에 대한 비소유(non-owning) View 클래스
 */
class SE_CORE_API StringView
{
public:
    using CharType = char;
    using SizeType = usize;

    using IteratorType = const CharType*;
    using ConstIteratorType = const CharType*;

public:
    constexpr StringView() noexcept = default;

    /** null-terminated 문자열로부터 View를 생성합니다. */
    template <typename T>
    requires (!std::same_as<std::remove_cvref_t<T>, String>)
        && std::convertible_to<T, const CharType*>
        && (!std::is_array_v<std::remove_cvref_t<T>>)
    constexpr StringView(T str) noexcept
        : data_ptr(str)
        , data_len(str ? StrLen(str) : 0)
    {
    }

    /** 포인터와 길이로부터 View를 생성합니다. */
    constexpr StringView(const CharType* str, SizeType len) noexcept
        : data_ptr(str)
        , data_len(len)
    {
    }

    /** 리터럴 문자열 및 고정 크기 배열로부터 View를 생성합니다. */
    template <SizeType N>
        requires (N > 0)
    constexpr StringView(const CharType (&str)[N]) noexcept
        : data_ptr(str)
        , data_len(N - 1) // null-terminator 제외
    {
        SE_ASSERT(
            str[N - 1] == '\0',
            "StringView(const char(&)[N]) requires a null-terminated array. "
            "If you are using a raw buffer without a null-terminator, "
            "use StringView(const char* ptr, SizeType len) instead."
        );
    }

    /** std::string_view로부터 변환합니다. */
    constexpr StringView(std::string_view sv) noexcept
        : data_ptr(sv.data())
        , data_len(sv.size())
    {
    }

    /** String으로부터 View를 생성합니다. */
    StringView(const String& str) noexcept;

    constexpr StringView(const StringView&) noexcept = default;
    constexpr StringView& operator=(const StringView&) noexcept = default;
    constexpr StringView(StringView&&) noexcept = default;
    constexpr StringView& operator=(StringView&&) noexcept = default;

public:
    /** 인덱스 위치의 문자를 반환합니다. (범위 검사 없음) */
    [[nodiscard]] constexpr const CharType& operator[](SizeType idx) const noexcept
    {
        return data_ptr[idx];
    }

    /** 인덱스 위치의 문자를 반환합니다. (범위 검사 있음) */
    [[nodiscard]] constexpr Optional<const CharType&> At(SizeType idx) const noexcept
    {
        if (idx >= data_len)
        {
            return NullOpt;
        }
        return data_ptr[idx];
    }

    /** 첫 번째 문자를 반환합니다. */
    [[nodiscard]] constexpr Optional<const CharType&> Front() const noexcept
    {
        if (data_len == 0)
        {
            return NullOpt;
        }
        return data_ptr[0];
    }

    /** 첫 번째 문자를 반환합니다. (범위 검사 없음) */
    [[nodiscard]] constexpr const CharType& FrontChecked() const noexcept
    {
        SE_ASSERT(data_len > 0, "FrontChecked() called on empty StringView");
        return data_ptr[0];
    }

    /** 마지막 문자를 반환합니다. */
    [[nodiscard]] constexpr Optional<const CharType&> Back() const noexcept
    {
        if (data_len == 0)
        {
            return NullOpt;
        }
        return data_ptr[data_len - 1];
    }

    /** 마지막 문자를 반환합니다. (범위 검사 없음) */
    [[nodiscard]] constexpr const CharType& BackChecked() const noexcept
    {
        SE_ASSERT(data_len > 0, "BackChecked() called on empty StringView");
        return data_ptr[data_len - 1];
    }

    /** 데이터 포인터를 반환합니다. */
    [[nodiscard]] constexpr const CharType* Data() const noexcept { return data_ptr; }

public:
    /** 바이트 길이를 반환합니다. */
    [[nodiscard]] constexpr SizeType ByteLen() const noexcept { return data_len; }

    /** View가 비어있는지 확인합니다. */
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return data_len == 0; }

public:
    [[nodiscard]] constexpr ConstIteratorType begin() const noexcept { return data_ptr; }
    [[nodiscard]] constexpr ConstIteratorType end() const noexcept { return data_ptr + data_len; }
    [[nodiscard]] constexpr ConstIteratorType cbegin() const noexcept { return data_ptr; }
    [[nodiscard]] constexpr ConstIteratorType cend() const noexcept { return data_ptr + data_len; }

public:
    /** View 앞에서 n 바이트를 제거합니다. */
    constexpr void RemovePrefix(SizeType n) noexcept
    {
        data_ptr += n;
        data_len -= n;
    }

    /** View 뒤에서 n 바이트를 제거합니다. */
    constexpr void RemoveSuffix(SizeType n) noexcept
    {
        data_len -= n;
    }

    /** 문자열 앞뒤의 공백을 제거한 새 View를 반환합니다. */
    [[nodiscard]] constexpr StringView Trim() const noexcept
    {
        if (IsEmpty())
        {
            return {};
        }

        SizeType start = 0;
        SizeType end = data_len;

        // 앞쪽 공백 스킵
        while (start < end && IsWhitespace(data_ptr[start]))
        {
            start++;
        }

        // 뒤쪽 공백 스킵
        while (end > start && IsWhitespace(data_ptr[end - 1]))
        {
            end--;
        }

        return Substr(start, end - start);
    }

    /** 문자열 앞쪽(왼쪽)의 공백만 제거합니다. */
    [[nodiscard]] constexpr StringView TrimStart() const noexcept
    {
        if (IsEmpty())
        {
            return {};
        }

        SizeType start = 0;
        while (start < data_len && IsWhitespace(data_ptr[start]))
        {
            start++;
        }

        return Substr(start, data_len - start);
    }

    /** 문자열 뒤쪽(오른쪽)의 공백만 제거합니다. */
    [[nodiscard]] constexpr StringView TrimEnd() const noexcept
    {
        if (IsEmpty())
        {
            return {};
        }

        SizeType end = data_len;
        while (end > 0 && IsWhitespace(data_ptr[end - 1]))
        {
            end--;
        }

        return Substr(0, end);
    }

public:
    /** 부분 문자열 View를 반환합니다. */
    [[nodiscard]] constexpr StringView Substr(SizeType pos, SizeType count = static_cast<SizeType>(-1)) const noexcept
    {
        const SizeType actual_pos = std::min(pos, data_len);
        const SizeType actual_count = std::min(count, data_len - actual_pos);
        return { data_ptr + actual_pos, actual_count };
    }

    /** 특정 문자열로 시작하는지 확인합니다. */
    [[nodiscard]] constexpr bool StartsWith(StringView prefix) const noexcept
    {
        if (prefix.data_len > data_len)
        {
            return false;
        }
        return Compare(data_ptr, prefix.data_ptr, prefix.data_len) == 0;
    }

    /** 특정 문자로 시작하는지 확인합니다. */
    [[nodiscard]] constexpr bool StartsWith(CharType c) const noexcept
    {
        return data_len > 0 && data_ptr[0] == c;
    }

    /** 특정 문자열로 끝나는지 확인합니다. */
    [[nodiscard]] constexpr bool EndsWith(StringView suffix) const noexcept
    {
        if (suffix.data_len > data_len)
        {
            return false;
        }
        return Compare(data_ptr + data_len - suffix.data_len, suffix.data_ptr, suffix.data_len) == 0;
    }

    /** 특정 문자로 끝나는지 확인합니다. */
    [[nodiscard]] constexpr bool EndsWith(CharType c) const noexcept
    {
        return data_len > 0 && data_ptr[data_len - 1] == c;
    }

    /** 특정 문자열이 포함되어 있는지 확인합니다. */
    [[nodiscard]] constexpr bool Contains(StringView sv) const noexcept
    {
        return Find(sv).HasValue();
    }

    /** 특정 문자가 포함되어 있는지 확인합니다. */
    [[nodiscard]] constexpr bool Contains(CharType c) const noexcept
    {
        return Find(c).HasValue();
    }

    /** 특정 문자를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> Find(CharType c, SizeType pos = 0) const noexcept
    {
        for (SizeType i = pos; i < data_len; ++i)
        {
            if (data_ptr[i] == c)
            {
                return i;
            }
        }
        return NullOpt;
    }

    /** 특정 문자열을 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> Find(StringView sv, SizeType pos = 0) const noexcept
    {
        if (sv.data_len == 0)
        {
            return pos <= data_len ? Optional{ pos } : NullOpt;
        }
        if (sv.data_len > data_len)
        {
            return NullOpt;
        }

        for (SizeType i = pos; i <= data_len - sv.data_len; ++i)
        {
            if (Compare(data_ptr + i, sv.data_ptr, sv.data_len) == 0)
            {
                return i;
            }
        }
        return NullOpt;
    }

    /** 마지막으로 특정 문자가 나타나는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindLast(CharType c, SizeType pos = static_cast<SizeType>(-1)) const noexcept
    {
        if (data_len == 0)
        {
            return NullOpt;
        }

        const SizeType start = (pos >= data_len) ? data_len - 1 : pos;
        for (SizeType i = start + 1; i > 0; --i)
        {
            if (data_ptr[i - 1] == c)
            {
                return i - 1;
            }
        }
        return NullOpt;
    }

    /** 마지막으로 특정 문자열이 나타나는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindLast(StringView sv, SizeType pos = static_cast<SizeType>(-1)) const noexcept
    {
        if (sv.data_len == 0)
        {
            return pos <= data_len ? Optional{ std::min(pos, data_len) } : NullOpt;
        }
        if (sv.data_len > data_len)
        {
            return NullOpt;
        }

        const SizeType start = std::min(pos, data_len - sv.data_len);
        for (SizeType i = start + 1; i > 0; --i)
        {
            if (Compare(data_ptr + i - 1, sv.data_ptr, sv.data_len) == 0)
            {
                return i - 1;
            }
        }
        return NullOpt;
    }

    /** 주어진 문자들 중 하나가 처음으로 나타나지 않는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindFirstNotOf(StringView chars, SizeType pos = 0) const noexcept
    {
        for (SizeType i = pos; i < data_len; ++i)
        {
            bool found = false;
            for (SizeType j = 0; j < chars.data_len; ++j)
            {
                if (data_ptr[i] == chars.data_ptr[j])
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return i;
            }
        }
        return NullOpt;
    }

    /** 주어진 문자들 중 하나가 마지막으로 나타나지 않는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindLastNotOf(StringView chars, SizeType pos = static_cast<SizeType>(-1)) const noexcept
    {
        if (data_len == 0)
        {
            return NullOpt;
        }

        const SizeType start = (pos >= data_len) ? data_len - 1 : pos;
        for (SizeType i = start + 1; i > 0; --i)
        {
            bool found = false;
            for (SizeType j = 0; j < chars.data_len; ++j)
            {
                if (data_ptr[i - 1] == chars.data_ptr[j])
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return i - 1;
            }
        }
        return NullOpt;
    }

    /** 주어진 문자들 중 하나가 처음으로 나타나는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindFirstOf(StringView chars, SizeType pos = 0) const noexcept
    {
        for (SizeType i = pos; i < data_len; ++i)
        {
            for (SizeType j = 0; j < chars.data_len; ++j)
            {
                if (data_ptr[i] == chars.data_ptr[j])
                {
                    return i;
                }
            }
        }
        return NullOpt;
    }

    /** 특정 문자가 처음으로 나타나는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindFirstOf(CharType c, SizeType pos = 0) const noexcept
    {
        return Find(c, pos);
    }

    /** 주어진 문자들 중 하나가 마지막으로 나타나는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindLastOf(StringView chars, SizeType pos = static_cast<SizeType>(-1)) const noexcept
    {
        if (data_len == 0)
        {
            return NullOpt;
        }

        const SizeType start = (pos >= data_len) ? data_len - 1 : pos;
        for (SizeType i = start + 1; i > 0; --i)
        {
            for (SizeType j = 0; j < chars.data_len; ++j)
            {
                if (data_ptr[i - 1] == chars.data_ptr[j])
                {
                    return i - 1;
                }
            }
        }
        return NullOpt;
    }

    /** 특정 문자가 마지막으로 나타나는 위치를 찾습니다. */
    [[nodiscard]] constexpr Optional<SizeType> FindLastOf(CharType c, SizeType pos = static_cast<SizeType>(-1)) const noexcept
    {
        return FindLast(c, pos);
    }

public:
    /** std::string_view로 변환합니다. */
    [[nodiscard]] constexpr operator std::string_view() const noexcept
    {
        return { data_ptr, data_len };
    }

    /** String 객체를 생성합니다. */
    [[nodiscard]] String ToString() const;

public:
    [[nodiscard]] constexpr bool operator==(StringView other) const noexcept
    {
        if (data_len != other.data_len)
        {
            return false;
        }
        return Compare(data_ptr, other.data_ptr, data_len) == 0;
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(StringView other) const noexcept
    {
        const SizeType min_len = std::min(data_len, other.data_len);
        const int32 result = Compare(data_ptr, other.data_ptr, min_len);
        if (result < 0)
        {
            return std::strong_ordering::less;
        }
        if (result > 0)
        {
            return std::strong_ordering::greater;
        }
        if (data_len < other.data_len)
        {
            return std::strong_ordering::less;
        }
        if (data_len > other.data_len)
        {
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

private:
    [[nodiscard]] static constexpr bool IsWhitespace(char c) noexcept
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }

    [[nodiscard]] static constexpr SizeType StrLen(const CharType* str) noexcept
    {
        SizeType len = 0;
        while (str[len] != '\0')
        {
            ++len;
        }
        return len;
    }

    [[nodiscard]] static constexpr int32 Compare(const CharType* lhs, const CharType* rhs, SizeType len) noexcept
    {
        for (SizeType i = 0; i < len; ++i)
        {
            if (static_cast<uint8>(lhs[i]) < static_cast<uint8>(rhs[i]))
            {
                return -1;
            }
            if (static_cast<uint8>(lhs[i]) > static_cast<uint8>(rhs[i]))
            {
                return 1;
            }
        }
        return 0;
    }

private:
    const CharType* data_ptr = nullptr;
    SizeType data_len = 0;
};
} // namespace se


template <>
struct std::hash<se::StringView>
{
    [[nodiscard]] size_t operator()(se::StringView sv) const noexcept
    {
        return std::hash<std::string_view>{}(std::string_view(sv.Data(), sv.ByteLen()));
    }
};

template <>
struct std::formatter<se::StringView> : std::formatter<std::string_view>
{
    auto format(se::StringView sv, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(
            std::string_view(sv.Data(), sv.ByteLen()), ctx
        );
    }
};
