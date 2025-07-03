export module SimpleEngine.Utility:StringUtils;
import std;


export namespace se::utility::string_utils
{
[[nodiscard]] std::u8string ToU8String(std::string_view in_str);
[[nodiscard]] std::u8string ToU8String(std::wstring_view in_str);
[[nodiscard]] std::u8string ToU8String(std::u16string_view in_str);
[[nodiscard]] std::u8string ToU8String(std::u32string_view in_str);

[[nodiscard]] std::u8string ToU8UpperCase(std::u8string_view in_str);
[[nodiscard]] std::u8string ToU8LowerCase(std::u8string_view in_str);
}
