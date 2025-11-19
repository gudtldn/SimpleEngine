#pragma once
#include <ranges>
#include <string_view>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::utility::string
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
    requires std::convertible_to<std::ranges::range_value_t<Rng>, std::string_view>
[[nodiscard]] BaseString<Allocator> Join(Rng&& range, std::string_view separator)
{
    return BaseString<Allocator>::FromRange(range | std::views::join_with(separator));
}
}
