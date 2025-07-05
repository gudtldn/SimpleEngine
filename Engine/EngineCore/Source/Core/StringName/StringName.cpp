module SimpleEngine.Core;
import :StringName;
import :StringName.StringNamePool;

import SimpleEngine.Utility;


namespace se::core::string_name
{
StringName StringName::None = StringName{};


StringName::StringName(const char8* in_str)
    : StringName(std::u8string_view(in_str))
{
}

StringName::StringName(std::u8string_view in_str)
{
    StringNamePool& pool = StringNamePool::Get();
    const auto [temp_display_hash, temp_comparison_hash] = pool.FindOrEmplace(in_str);

    display_hash = temp_display_hash;
    comparison_hash = temp_comparison_hash;
}

std::u8string StringName::ToString() const
{
    const StringNamePool& pool = StringNamePool::Get();
    const StringNameEntry& entry = pool.Resolve(display_hash);
    return entry.name;
}
}
