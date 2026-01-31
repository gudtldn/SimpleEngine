#include "StringNamePool.h"

#include <mutex>
#include <utility>

#include "Utility/Hash.h"


namespace
{
bool IsNoneString(se::StringView view)
{
    if (view.ByteLen() != 4)
    {
        return false;
    }

    return (view[0] == 'n' || view[0] == 'N')
        && (view[1] == 'o' || view[1] == 'O')
        && (view[2] == 'n' || view[2] == 'N')
        && (view[3] == 'e' || view[3] == 'E');
}
}  // namespace


namespace se
{
StringStorage::StringStorage()
{
    AllocateNewBlock();
}

StringStorage::~StringStorage() = default;

const char* StringStorage::Store(StringView view)
{
    const usize len = view.ByteLen();
    const usize size_needed = len + 1; // null-terminator 포함

    // 현재 블록에 공간이 부족하면 새 블록 할당
    if (current_offset + size_needed > BLOCK_SIZE)
    {
        // 만약 단일 문자열이 블록 크기보다 크다면, 맞춤형 블록 생성 (기존 블록과는 별개로 관리)
        if (size_needed > BLOCK_SIZE)
        {
            auto huge_block = std::make_unique<char[]>(size_needed);
            char* ptr = huge_block.get();
            std::memcpy(ptr, view.Data(), len);
            ptr[len] = '\0';

            blocks.Push(std::move(huge_block));
            return ptr;
        }

        AllocateNewBlock();
    }

    // 현재 블록의 남은 공간에 복사
    char* dest = current_block_ptr + current_offset;
    std::memcpy(dest, view.Data(), len);
    dest[len] = '\0';

    current_offset += size_needed;
    return dest;
}

void StringStorage::AllocateNewBlock()
{
    auto new_block = std::make_unique<char[]>(BLOCK_SIZE);
    current_block_ptr = new_block.get();
    current_offset = 0;
    blocks.Push(std::move(new_block));
}

StringNamePool& StringNamePool::Get()
{
    static StringNamePool instance;
    return instance;
}

Optional<const StringNameEntry&> StringNamePool::Find(StringView view) const
{
    if (view.IsEmpty() || IsNoneString(view))
    {
        return std::nullopt;
    }

    const uint64 comparison_hash = utility::FNV_Hash_CaseInsensitive(view);

    std::shared_lock lock(string_pool_mutex);
    return lookup_map.Find(comparison_hash)
        .AndThen([](const StringNameEntry* entry_ptr) -> Optional<const StringNameEntry&>
        {
            return *entry_ptr;
        });
}

const StringNameEntry& StringNamePool::FindOrEmplace(StringView view)
{
    if (view.IsEmpty() || IsNoneString(view))
    {
        static constexpr StringNameEntry NoneEntry = {
            .display_name = nullptr,
            .comparison_hash = 0,
            .length = 0,
        };
        return NoneEntry;
    }

    // pool에 있는지 확인
    const uint64 display_hash = utility::FNV_Hash(view);
    {
        std::shared_lock lock(string_pool_mutex);
        if (const Optional entry_opt = entry_pool.Find(display_hash))
        {
            return *entry_opt;
        }
    }

    // 없으면 만들기
    const uint64 comparison_hash = utility::FNV_Hash_CaseInsensitive(view);
    std::unique_lock lock(string_pool_mutex);

    // Double Check
    if (const Optional entry_opt = entry_pool.Find(display_hash))
    {
        return *entry_opt;
    }

    const StringNameEntry& new_entry = entry_pool.Insert(display_hash, {
        .display_name = string_storage.Store(view),
        .comparison_hash = comparison_hash,
        .length = static_cast<uint32>(view.ByteLen()),
    });

    lookup_map.Entry(comparison_hash).OrInsert(&new_entry);
    return new_entry;
}
}  // namespace se
