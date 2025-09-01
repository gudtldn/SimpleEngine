module SE.Types;
import :StringName;
import :StringName.StringNamePool;

import SE.Utility;


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

    if constexpr (se::utility::IS_DEBUG_BUILD)
    {
        debug_entry_ptr = display_hash == 0 ? nullptr : &pool.Resolve(display_hash);
    }
}

std::u8string StringName::ToString() const
{
    if (display_hash == 0 && comparison_hash == 0)
    {
        return u8"None";
    }

    const StringNamePool& pool = StringNamePool::Get();
    const StringNameEntry& entry = pool.Resolve(display_hash);
    return entry.name;
}
