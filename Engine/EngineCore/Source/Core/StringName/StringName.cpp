module SimpleEngine.Core.StringName;

import SimpleEngine.Core.Hash;
import SimpleEngine.Utility;


namespace se::core
{
StringName::StringName(const char8* in_str)
    : StringName(std::u8string_view(in_str))
{
}

StringName::StringName(std::u8string_view in_str)
{
    // TODO: StringNamePool에 등록해야함
    display_hash = hash::FowlerNollVoHash(in_str);
    comparison_hash = hash::FowlerNollVoHash(string_utils::ToU8LowerCase(in_str));
}

std::u8string StringName::ToString() const
{
    // TODO: Implements this
    std::unreachable();
}
}
