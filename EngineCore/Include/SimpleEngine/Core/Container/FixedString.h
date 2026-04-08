// ReSharper disable CppMemberFunctionMayBeStatic
#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/StringView.h"

#include <algorithm>


namespace se
{
/**
 * 컴파일 타임 문자열 리터럴을 값으로 저장하는 Structural Type
 * @tparam N null-terminator를 포함한 문자 배열 크기
 */
template <usize N>
class FixedString
{
    static_assert(N > 0, "FixedString must at least contain a null terminator.");

public:
    using ValueType = char;

    using IteratorType = char*;
    using ConstIteratorType = const char*;

public:
    constexpr FixedString() = default;

    /** 문자열 리터럴 또는 고정 크기 char 배열로부터 생성합니다. */
    constexpr FixedString(const char (&str)[N])
    {
        std::copy_n(str, N, data.begin());
    }

    /** null-terminator를 제외한 문자열 길이를 반환합니다. */
    [[nodiscard]] constexpr usize Len() const { return N - 1; }

    /** 문자열이 비어있는지 확인합니다. */
    [[nodiscard]] constexpr bool IsEmpty() const { return N <= 1; }

    /** 데이터에 직접 접근합니다. */
    [[nodiscard]] constexpr const char* Data() const { return data.Data(); }
    [[nodiscard]] constexpr char* Data() { return data.Data(); }

    /** 개별 문자에 직접 접근합니다. */
    [[nodiscard]] constexpr const char& operator[](usize index) const { return data[index]; }

public:
    [[nodiscard]] constexpr IteratorType begin() { return data.begin(); }
    [[nodiscard]] constexpr IteratorType end() { return data.end(); }
    [[nodiscard]] constexpr ConstIteratorType begin() const { return data.begin(); }
    [[nodiscard]] constexpr ConstIteratorType end() const { return data.end(); }

    /** StringView로 변환합니다. */
    [[nodiscard]] constexpr operator StringView() const { return { Data(), Len() }; }

    /** 다른 FixedString과 비교합니다. */
    template <usize M>
    [[nodiscard]] constexpr bool operator==(const FixedString<M>& other) const
    {
        if constexpr (N != M)
        {
            return false;
        }
        else
        {
            return std::ranges::equal(*this, other);
        }
    }

    [[nodiscard]] constexpr auto operator<=>(const FixedString&) const = default;

public:
    FixedArray<char, N> data{};
};

template <usize N>
FixedString(const char (&)[N]) -> FixedString<N>;
}  // namespace se
