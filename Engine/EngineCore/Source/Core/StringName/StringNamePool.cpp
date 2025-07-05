module SimpleEngine.Core;
import :StringName.StringNamePool;
import :Hash;

import SimpleEngine.Utility;
import <cassert>;


namespace
{
// TODO: shared_mutex로 변경
std::mutex string_pool_mutex;
}


StringNamePool& StringNamePool::Get()
{
    static StringNamePool instance;
    return instance;
}

const StringNameEntry& StringNamePool::Resolve(uint64 hash) const
{
    if (const auto it = display_string_pool.find(hash); it != display_string_pool.end())
    {
        return it->second;
    }
    std::unreachable();
}

uint64 StringNamePool::FindOrStore(const std::u8string_view& view)
{
    const uint64 display_hash = se::core::hash::FowlerNollVoHash(view);
    if (const auto it = display_string_pool.find(display_hash); it != display_string_pool.end())
    {
        return it->second.comparison_hash;
    }

    const std::u8string lower_case_str = se::utility::string_utils::ToU8LowerCase(view);
    const uint64 comparison_hash = se::core::hash::FowlerNollVoHash(lower_case_str);

    StringNameEntry comparison_entry = { lower_case_str, comparison_hash };
    {
        std::scoped_lock lock(string_pool_mutex);

        if (!comparison_string_pool.contains(comparison_hash))
        {
            comparison_string_pool.emplace(comparison_hash, comparison_entry);
        }

        StringNameEntry display_entry = { view, comparison_hash };
        display_string_pool.emplace(display_hash, display_entry);
    }
    return display_hash;
}

StringNamePool::StringNamePool()
{
    // TODO: None 기본값 설정
}

