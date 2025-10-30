#pragma once
#include <string_view>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::utility::string
{
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
    using StringType = BaseString<Allocator>;
    using RangeValueType = std::ranges::range_value_t<Rng>;

    typename StringType::SizeType total_byte_len = 0;
    typename StringType::SizeType element_count = 0;

    for (const RangeValueType& element : range)
    {
        total_byte_len += std::string_view{ element }.size();
        ++element_count;
    }

    // 구분자가 삽입될 횟수 = 요소 개수 - 1 (요소가 0개 또는 1개일 경우 0)
    if (element_count > 0)
    {
        total_byte_len += separator.length() * (element_count - 1);
    }

    StringType result;
    result.Reserve(total_byte_len);

    bool first = true;
    for (const RangeValueType& element : range)
    {
        if (!first)
        {
            result.Append(separator);
        }
        else
        {
            first = false;
        }

        result.Append(std::string_view{ element });
    }

    return result;
}
}
