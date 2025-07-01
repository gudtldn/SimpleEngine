export module SimpleEngine.Core.StringName;
import :StringNamePool;

import SimpleEngine.Platform.Types;
import std;


namespace se::core
{
/**
 *
 */
export class StringName
{
public:
    StringName() = default;
    StringName(std::u8string_view in_str);

    std::u8string ToString() const;
    uint64 GetDisplayHash() const { return display_hash; }
    uint64 GetComparisonHash() const { return comparison_hash; }

public:
    bool operator==(const StringName&) const = default;
    bool operator!=(const StringName&) const = default;

private:
    uint64 display_hash = 0;
    uint64 comparison_hash = 0;
};
}

template <>
struct std::hash<se::core::StringName>
{
    uint64 operator()(const se::core::StringName& key) const noexcept
    {
        return hash<uint64>()(key.GetComparisonHash());
    }
};
