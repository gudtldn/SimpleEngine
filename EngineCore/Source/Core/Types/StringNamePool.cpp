#include "StringNamePool.h"

#include <mutex>
#include <utility>

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
}


StringNamePool& StringNamePool::Get()
{
    static StringNamePool instance;
    return instance;
}

const StringNameEntry& StringNamePool::Resolve(uint64 hash) const
{
    std::shared_lock lock(string_pool_mutex);

    if (const Optional display_pool_opt = display_string_pool.Find(hash))
    {
        return *display_pool_opt;
    }

    std::unreachable();
}

StringNameHashes StringNamePool::Find(std::string_view view) const
{
    if (view.empty() || IsNoneString(view))
    {
        return { 0, 0 };
    }

    {
        const se::String lower_case_str = se::String{ view }.ToLower();
        const uint64 comparison_hash = se::utility::FNV_Hash(lower_case_str);

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
    const uint64 display_hash = se::utility::FNV_Hash(view);
    {
        std::shared_lock lock(string_pool_mutex);

        if (const Optional display_pool_opt = display_string_pool.Find(display_hash))
        {
            return { display_hash, display_pool_opt->comparison_hash };
        }
    }

    // 없으면 만들기
    const se::String lower_case_str = se::String{ view }.ToLower();
    const uint64 comparison_hash = se::utility::FNV_Hash(lower_case_str);
    {
        std::unique_lock lock(string_pool_mutex);

        // double check
        if (const Optional display_pool_opt = display_string_pool.Find(display_hash))
        {
            return { display_hash, display_pool_opt->comparison_hash };
        }

        // pool에 entry를 등록, 처음에 추가된 이름을 comparison의 이름으로 설정
        comparison_hash_to_display_hash.Entry(comparison_hash).OrInsert(display_hash);
        display_string_pool.Emplace(display_hash, StringNameEntry(view, comparison_hash));
    }

    return { display_hash, comparison_hash };
}
