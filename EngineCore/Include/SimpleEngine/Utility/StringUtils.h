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
}
