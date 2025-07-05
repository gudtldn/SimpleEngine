export module SimpleEngine.Core:StringName;

import SimpleEngine.Types;
import std;


namespace se::core::string_name
{
/**
 *
 */
export class StringName
{
public:
    StringName() = default;
    StringName(const char8* in_str);
    StringName(std::u8string_view in_str);

    std::u8string ToString() const;
    uint64 GetDisplayHash() const { return display_hash; }
    uint64 GetComparisonHash() const { return comparison_hash; }

public:
    constexpr static size_t MAX_LENGTH = 256;
    static StringName None;

public:
    inline bool operator==(const StringName& other) const
    {
        return comparison_hash == other.comparison_hash;
    }

    inline bool operator!=(const StringName& other) const
    {
        return comparison_hash != other.comparison_hash;
    }

private:
    uint64 display_hash = 0;
    uint64 comparison_hash = 0;
};
}

template <>
struct std::hash<se::core::string_name::StringName>
{
    uint64 operator()(const se::core::string_name::StringName& key) const noexcept
    {
        return hash<uint64>()(key.GetComparisonHash());
    }
};
