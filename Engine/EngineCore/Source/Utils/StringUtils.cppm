export module SimpleEngine.Utils:StringUtils;
import std;


export namespace se::string_utils
{

// u8string Convertor
[[nodiscard]] std::u8string ToU8String(std::string_view in_str);
[[nodiscard]] std::u8string ToU8String(std::wstring_view in_str);
[[nodiscard]] std::u8string ToU8String(std::u16string_view in_str);
[[nodiscard]] std::u8string ToU8String(std::u32string_view in_str);

}
