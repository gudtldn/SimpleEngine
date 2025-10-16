#pragma once
#include <string_view>

#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::utility::string
{
[[nodiscard]] SE_CORE_API u8string ToU8String(std::string_view in_str);
[[nodiscard]] SE_CORE_API u8string ToU8String(std::wstring_view in_str);
[[nodiscard]] SE_CORE_API u8string ToU8String(std::u16string_view in_str);
[[nodiscard]] SE_CORE_API u8string ToU8String(std::u32string_view in_str);

[[nodiscard]] SE_CORE_API u8string ToU8UpperCase(std::u8string_view in_str);
[[nodiscard]] SE_CORE_API u8string ToU8LowerCase(std::u8string_view in_str);
}
