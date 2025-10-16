#include "StringNamePool.h"

#include <mutex>
#include <utility>

#include "Utility/Hash.h"
#include "Utility/StringUtils.h"


namespace
{
bool IsNoneString(const std::u8string_view& view)
{
    if (view.length() == 4)
    {
        return (view[0] == u8'n' || view[0] == u8'N')
            && (view[1] == u8'o' || view[1] == u8'O')
            && (view[2] == u8'n' || view[2] == u8'N')
            && (view[3] == u8'e' || view[3] == u8'E');
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

    if (const auto it = display_string_pool.find(hash); it != display_string_pool.end())
    {
        return it->second;
    }

    std::unreachable();
}

StringNameHashes StringNamePool::FindOrEmplace(const std::u8string_view& view)
{
    if (view.empty() || IsNoneString(view))
    {
        return { 0, 0 };
    }

    // display string pool에 있는지 확인
    const uint64 display_hash = se::utility::FNV_Hash(view);
    {
        std::shared_lock lock(string_pool_mutex);

        if (const auto it = display_string_pool.find(display_hash); it != display_string_pool.end())
        {
            return { display_hash, it->second.comparison_hash };
        }
    }

    // 없으면 만들기
    const se::u8string lower_case_str = se::utility::string::ToU8LowerCase(view);
    const uint64 comparison_hash = se::utility::FNV_Hash(lower_case_str);
    {
        std::unique_lock lock(string_pool_mutex);

        // double check
        if (const auto it = display_string_pool.find(display_hash); it != display_string_pool.end())
        {
            return { display_hash, it->second.comparison_hash };
        }

        // pool에 entry를 등록
        if (!comparison_string_pool.contains(comparison_hash))
        {
            comparison_string_pool.emplace(comparison_hash, StringNameEntry{ lower_case_str, comparison_hash });
        }
        display_string_pool.emplace(display_hash, StringNameEntry{ view, comparison_hash });
    }

    return { display_hash, comparison_hash };
}
