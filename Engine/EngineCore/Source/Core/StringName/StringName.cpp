module SimpleEngine.Core;
import :StringName;

import :Hash;
import SimpleEngine.Utility;


namespace se::core::string_name
{
StringName::StringName(const char8* in_str)
    : StringName(std::u8string_view(in_str))
{
}

StringName::StringName(std::u8string_view in_str)
{
    // TODO: StringNamePool에 등록해야함
    display_hash = hash::FowlerNollVoHash(in_str);
    comparison_hash = hash::FowlerNollVoHash(utility::string_utils::ToU8LowerCase(in_str));
}

std::u8string StringName::ToString() const
{
    // TODO: Implements this
    std::unreachable();
}
}
