#pragma once
#include <ranges>
#include <string_view>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"


namespace se::utility
{
[[nodiscard]] SE_CORE_API String ToString(std::string_view in_str);
[[nodiscard]] SE_CORE_API String ToString(std::wstring_view in_str);
[[nodiscard]] SE_CORE_API String ToString(std::u8string_view in_str);
[[nodiscard]] SE_CORE_API String ToString(std::u16string_view in_str);
[[nodiscard]] SE_CORE_API String ToString(std::u32string_view in_str);

template <
    std::ranges::input_range Rng,
    typename Allocator = String::AllocatorType
>
    requires std::convertible_to<std::ranges::range_value_t<Rng>, StringView>
[[nodiscard]] BaseString<Allocator> Join(Rng&& range, StringView separator)
{
    return BaseString<Allocator>::FromRange(range | std::views::join_with(std::string_view{ separator }));
}
}
