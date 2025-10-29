#include "Core/Types/StringName.h"
#include "StringNamePool.h"


StringName StringName::None = StringName{};

StringName StringName::Find(const char* in_str)
{
    return Find(std::string_view(in_str));
}

StringName StringName::Find(std::string_view in_str)
{
    const StringNamePool& pool = StringNamePool::Get();
    const auto [temp_display_hash, temp_comparison_hash] = pool.Find(in_str);

    StringName result;
    result.display_hash = temp_display_hash;
    result.comparison_hash = temp_comparison_hash;

#ifdef SE_DEBUG_BUILD
    result.debug_entry_ptr = result.display_hash == 0 ? nullptr : &pool.Resolve(result.display_hash);
#endif

    return result;
}

StringName::StringName(const char* in_str)
    : StringName(std::string_view{ in_str })
{
}

StringName::StringName(const se::String& in_str)
    : StringName(std::string_view{ in_str })
{
}

StringName::StringName(std::string_view in_str)
{
    StringNamePool& pool = StringNamePool::Get();
    const auto [temp_display_hash, temp_comparison_hash] = pool.FindOrEmplace(in_str);

    display_hash = temp_display_hash;
    comparison_hash = temp_comparison_hash;

#ifdef SE_DEBUG_BUILD
    debug_entry_ptr = display_hash == 0 ? nullptr : &pool.Resolve(display_hash);
#endif
}

se::String StringName::ToString() const
{
    if (display_hash == 0 && comparison_hash == 0)
    {
        return "None";
    }

    const StringNamePool& pool = StringNamePool::Get();
    const StringNameEntry& entry = pool.Resolve(display_hash);
    return entry.name;
}
