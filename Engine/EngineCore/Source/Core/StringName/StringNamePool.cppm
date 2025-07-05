export module SimpleEngine.Core:StringName.StringNamePool;
import :StringName;

import SimpleEngine.Types;
import std;


struct StringNameEntry
{
    StringNameEntry(const std::u8string_view& view, uint64 hash)
        : comparison_hash(hash)
        , length(static_cast<uint16>(view.length()))
    {
        std::memcpy(name, view.data(), sizeof(char8) * length);
        name[length] = '\0';
    }

    uint64 comparison_hash;
    uint16 length;
    char8 name[se::core::string_name::StringName::MAX_LENGTH];
};

class StringNamePool
{
public:
    static StringNamePool& Get();

    const StringNameEntry& Resolve(uint64 hash) const;
    uint64 FindOrStore(const std::u8string_view& view);

private:
    std::unordered_map<uint64, StringNameEntry> display_string_pool;
    std::unordered_map<uint64, StringNameEntry> comparison_string_pool;

private:
    StringNamePool();
    ~StringNamePool() = default;

    // 이동 & 복사 생성자 제거
    StringNamePool(const StringNamePool&) = delete;
    StringNamePool& operator=(const StringNamePool&) = delete;
    StringNamePool(StringNamePool&&) = delete;
    StringNamePool& operator=(StringNamePool&&) = delete;
};
