export module SimpleEngine.Types:StringName;
import :PlatformTypes;

import std;


/**
 * 빠른 비교 및 조회를 위해 문자열을 ID로 관리하는 클래스
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

#ifdef _DEBUG
    const void* debug_entry_ptr = nullptr;
#endif
};


template <>
struct std::hash<StringName>
{
    uint64 operator()(const StringName& key) const noexcept
    {
        return hash<uint64>()(key.GetComparisonHash());
    }
};
