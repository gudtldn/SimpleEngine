#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"

#include <ranges>
#include <string_view>


namespace se
{
/**
 * 문자열 관련 유틸리티 함수 모음
 */
struct SE_CORE_API StringUtils
{
    StringUtils() = delete;

    [[nodiscard]] static String ToString(std::string_view in_str);
    [[nodiscard]] static String ToString(std::wstring_view in_str);
    [[nodiscard]] static String ToString(std::u8string_view in_str);
    [[nodiscard]] static String ToString(std::u16string_view in_str);
    [[nodiscard]] static String ToString(std::u32string_view in_str);

    template <
        std::ranges::input_range Rng,
        typename Allocator = String::AllocatorType
    >
        requires std::convertible_to<std::ranges::range_value_t<Rng>, StringView>
    [[nodiscard]] static BaseString<Allocator> Join(Rng&& range, StringView separator)
    {
        return BaseString<Allocator>::FromRange(range | std::views::join_with(std::string_view{ separator }));
    }
};
}
