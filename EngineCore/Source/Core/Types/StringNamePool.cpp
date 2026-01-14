#include "StringNamePool.h"

#include <mutex>
#include <utility>

#include "Utility/Debug.h"
#include "Utility/Hash.h"


namespace
{
bool IsNoneString(std::string_view view)
{
    if (view.length() == 4)
    {
        return (view[0] == 'n' || view[0] == 'N')
            && (view[1] == 'o' || view[1] == 'O')
            && (view[2] == 'n' || view[2] == 'N')
            && (view[3] == 'e' || view[3] == 'E');
    }
    return false;
}
}  // namespace


namespace se
{
StringStorage::StringStorage()
{
    AllocateNewBlock();
}

StringStorage::~StringStorage() = default;

const char* StringStorage::Store(std::string_view view)
{
    const usize len = view.length();
    const usize size_needed = len + 1; // null-terminator 포함

    // 현재 블록에 공간이 부족하면 새 블록 할당
    if (current_offset + size_needed > BLOCK_SIZE)
    {
        // 만약 단일 문자열이 블록 크기보다 크다면, 맞춤형 블록 생성 (기존 블록과는 별개로 관리)
        if (size_needed > BLOCK_SIZE)
        {
            auto new_block = std::make_unique<char[]>(size_needed);
            char* ptr = new_block.get();
            std::memcpy(ptr, view.data(), len);
            ptr[len] = '\0';
            blocks.Push(std::move(new_block));
            return ptr;
        }

        AllocateNewBlock();
    }

    // 현재 블록의 남은 공간에 복사
    char* ptr = current_block + current_offset;
    std::memcpy(ptr, view.data(), len);
    ptr[len] = '\0';

    current_offset += size_needed;
    return ptr;
}

void StringStorage::AllocateNewBlock()
{
    auto new_block = std::make_unique<char[]>(BLOCK_SIZE);
    current_block = new_block.get();
    current_offset = 0;
    blocks.Push(std::move(new_block));
}

StringNamePool& StringNamePool::Get()
{
    static StringNamePool instance;
    return instance;
}

const StringNameEntry& StringNamePool::Resolve(uint64 hash) const
{
    std::shared_lock lock(string_pool_mutex);
    return display_string_pool.FindChecked(hash);
}

StringNameHashes StringNamePool::Find(std::string_view view) const
{
    if (view.empty() || IsNoneString(view))
    {
        return { 0, 0 };
    }

    {
        const String lower_case_str = String{ view }.ToLower();
        const uint64 comparison_hash = utility::FNV_Hash(lower_case_str);

        std::shared_lock lock(string_pool_mutex);
        if (const Optional comp2disp_hash_opt = comparison_hash_to_display_hash.Find(comparison_hash))
        {
            return { *comp2disp_hash_opt, comparison_hash };
        }
    }

    return { 0, 0 };
}

StringNameHashes StringNamePool::FindOrEmplace(std::string_view view)
{
    if (view.empty() || IsNoneString(view))
    {
        return { 0, 0 };
    }

    // display string pool에 있는지 확인
    const uint64 display_hash = utility::FNV_Hash(view);
    {
        std::shared_lock lock(string_pool_mutex);
        if (const Optional display_pool_opt = display_string_pool.Find(display_hash))
        {
            return { display_hash, display_pool_opt->comparison_hash };
        }
    }

    // 없으면 만들기
    const String lower_case_str = String{ view }.ToLower();
    const uint64 comparison_hash = utility::FNV_Hash(lower_case_str);

    {
        std::unique_lock lock(string_pool_mutex);

        // double check
        if (const Optional display_pool_opt = display_string_pool.Find(display_hash))
        {
            return { display_hash, display_pool_opt->comparison_hash };
        }

        // pool에 entry를 등록, 처음에 추가된 이름을 comparison의 이름으로 설정
        comparison_hash_to_display_hash.Entry(comparison_hash).OrInsert(display_hash);
        display_string_pool.Emplace(
            display_hash,
            StringNameEntry(
                string_storage.Store(view),
                static_cast<uint16>(view.length()),
                comparison_hash
            )
        );
    }

    return { display_hash, comparison_hash };
}
}  // namespace se
