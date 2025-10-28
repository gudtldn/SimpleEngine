#pragma once
#include <format>
#include <string_view>

#include "SimpleEngine/Core/Container/String.h"


// se::String에 대한 std::formatter 특수화
template <>
struct std::formatter<se::String, char> : std::formatter<std::string_view>
{
    auto format(const se::String& string, std::format_context& ctx) const
    {
        const std::string_view sv{ string };
        return std::formatter<std::string_view>::format(sv, ctx);
    }
};
