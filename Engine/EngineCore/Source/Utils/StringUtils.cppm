export module SimpleEngine.Utils:StringUtils;
import std;


export namespace se::string_utils
{

// u8string Convertor
[[nodiscard]] std::u8string ToU8String(const std::string& in_str);
[[nodiscard]] std::u8string ToU8String(const std::wstring& in_str);
[[nodiscard]] std::u8string ToU8String(const std::u16string& in_str);
[[nodiscard]] std::u8string ToU8String(const std::u32string& in_str);

}
