export module SE.Utility:StringUtils;

import SE.Types;
import std;


export namespace se::utility::string
{
[[nodiscard]] u8string ToU8String(std::string_view in_str);
[[nodiscard]] u8string ToU8String(std::wstring_view in_str);
[[nodiscard]] u8string ToU8String(std::u16string_view in_str);
[[nodiscard]] u8string ToU8String(std::u32string_view in_str);

[[nodiscard]] u8string ToU8UpperCase(std::u8string_view in_str);
[[nodiscard]] u8string ToU8LowerCase(std::u8string_view in_str);
}
