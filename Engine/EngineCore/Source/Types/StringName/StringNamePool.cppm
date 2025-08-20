export module SimpleEngine.Types:StringName.StringNamePool;
import :PlatformTypes;
import :StringName;

import std;


struct StringNameHashes
{
    uint64 display_hash = 0;
    uint64 comparison_hash = 0;
};

struct StringNameEntry
{
    StringNameEntry(const std::u8string_view& view, uint64 in_comparison_hash)
        : comparison_hash(in_comparison_hash)
        , length(static_cast<uint16>(view.length()))
    {
        std::memcpy(name, view.data(), sizeof(char8) * length);
        name[length] = '\0';
    }

    uint64 comparison_hash;
    uint16 length;
    char8 name[StringName::MAX_LENGTH];
};

class StringNamePool
{
public:
    static StringNamePool& Get();

    const StringNameEntry& Resolve(uint64 hash) const;
    StringNameHashes FindOrEmplace(const std::u8string_view& view);

private:
    mutable std::shared_mutex string_pool_mutex;

    std::unordered_map<uint64, StringNameEntry> display_string_pool;
    std::unordered_map<uint64, StringNameEntry> comparison_string_pool;

private:
    StringNamePool() = default;
    ~StringNamePool() = default;

    // 이동 & 복사 생성자 제거
    StringNamePool(const StringNamePool&) = delete;
    StringNamePool& operator=(const StringNamePool&) = delete;
    StringNamePool(StringNamePool&&) = delete;
    StringNamePool& operator=(StringNamePool&&) = delete;
};
