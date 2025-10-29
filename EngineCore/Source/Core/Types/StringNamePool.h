#pragma once
#include <cstring>
#include <shared_mutex>

#include "Core/Container/HashMap.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/StringName.h"

#include "tracy/Tracy.hpp"


struct StringNameHashes
{
    uint64 display_hash = 0;
    uint64 comparison_hash = 0;
};

struct StringNameEntry
{
    StringNameEntry(std::string_view view, uint64 in_comparison_hash)
        : comparison_hash(in_comparison_hash)
        , length(static_cast<uint16>(view.length()))
    {
        std::memcpy(name, view.data(), sizeof(char) * length);
        name[length] = '\0';
    }

    uint64 comparison_hash;
    uint16 length;
    char name[StringName::MAX_LENGTH];
};

class StringNamePool
{
private:
    StringNamePool() = default;

public:
    ~StringNamePool() = default;

    // 이동 & 복사 생성자 제거
    StringNamePool(const StringNamePool&) = delete;
    StringNamePool& operator=(const StringNamePool&) = delete;
    StringNamePool(StringNamePool&&) = delete;
    StringNamePool& operator=(StringNamePool&&) = delete;

public:
    static StringNamePool& Get();

    [[nodiscard]] const StringNameEntry& Resolve(uint64 hash) const;
    [[nodiscard]] StringNameHashes Find(std::string_view view) const;
    [[nodiscard]] StringNameHashes FindOrEmplace(std::string_view view);

private:
    mutable TracySharedLockable(std::shared_mutex, string_pool_mutex);

    se::HashMap<uint64, uint64> comparison_hash_to_display_hash;
    se::HashMap<uint64, StringNameEntry> display_string_pool;
};
