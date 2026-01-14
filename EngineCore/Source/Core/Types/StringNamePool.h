#pragma once
#include <cstring>
#include <memory>
#include <shared_mutex>

#include "Core/Container/HashMap.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/StringName.h"

#include "tracy/Tracy.hpp"


namespace se
{
struct StringNameHashes
{
    uint64 display_hash = 0;
    uint64 comparison_hash = 0;
};

struct StringNameEntry
{
    const char* name;
    uint16 length;
    uint64 comparison_hash;

    StringNameEntry(const char* in_name, uint16 in_length, uint64 in_comparison_hash)
        : name(in_name)
        , length(in_length)
        , comparison_hash(in_comparison_hash)
    {
    }
};

// TODO: 나중에 LinearAllocator 만들면 StringStorage 개선하기
class StringStorage
{
public:
    static constexpr usize BLOCK_SIZE = 64ULL * 1024; // 64KB

public:
    StringStorage();
    ~StringStorage();

    const char* Store(std::string_view view);

private:
    void AllocateNewBlock();

private:
    Array<std::unique_ptr<char[]>> blocks;
    char* current_block = nullptr;
    usize current_offset = 0;
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

    StringStorage string_storage;
    HashMap<uint64, uint64> comparison_hash_to_display_hash;
    HashMap<uint64, StringNameEntry> display_string_pool;
};
}  // namespace se
