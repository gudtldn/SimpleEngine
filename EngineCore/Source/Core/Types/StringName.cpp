#include "SimpleEngine/Core/Types/StringName.h"
#include "StringNamePool.h"


namespace se
{
StringName StringName::None = StringName{};

StringName StringName::Find(const char* in_str)
{
    return Find(StringView{ in_str });
}

StringName StringName::Find(StringView in_str)
{
    const StringNamePool& pool = StringNamePool::Get();
    return pool.Find(in_str)
        .Transform([](const StringNameEntry& entry)
        {
            StringName result;
            result.display_name = entry.display_name;
            result.comparison_hash = entry.comparison_hash;
            return result;
        })
        .ValueOr(None);
}

StringName::StringName(const char* in_str)
    : StringName(StringView{ in_str })
{
}

StringName::StringName(const String& in_str)
    : StringName(StringView{ in_str })
{
}

StringName::StringName(StringView in_str)
{
    StringNamePool& pool = StringNamePool::Get();
    const auto [pool_display_name, pool_comparison_hash, _] = pool.FindOrEmplace(in_str);

    display_name = pool_display_name;
    comparison_hash = pool_comparison_hash;
}

const char* StringName::CStr() const
{
    if (comparison_hash == 0 && display_name == nullptr)
    {
        return "None";
    }
    return display_name;
}

String StringName::ToString() const
{
    return { CStr() };
}
}  // namespace se
