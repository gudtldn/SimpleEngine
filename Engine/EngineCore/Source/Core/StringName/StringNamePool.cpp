module SimpleEngine.Core;
import :StringName.StringNamePool;
import :Hash;

import SimpleEngine.Utility;
import <cassert>;


namespace
{
// TODO: shared_mutex로 변경
std::mutex string_pool_mutex;

bool IsNoneString(const std::u8string_view& str)
{
    if (str.length() == 4)
    {
        return (str[0] == u8'n' || str[0] == u8'N')
            && (str[1] == u8'o' || str[1] == u8'O')
            && (str[2] == u8'n' || str[2] == u8'N')
            && (str[3] == u8'e' || str[3] == u8'E');
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

    const uint64 display_hash = se::core::hash::FowlerNollVoHash(view);
    if (const auto it = display_string_pool.find(display_hash); it != display_string_pool.end())
    {
        return { display_hash, it->second.comparison_hash };
    }

    const std::u8string lower_case_str = se::utility::string_utils::ToU8LowerCase(view);
    const uint64 comparison_hash = se::core::hash::FowlerNollVoHash(lower_case_str);

    {
        std::scoped_lock lock(string_pool_mutex);

        if (!comparison_string_pool.contains(comparison_hash))
        {
            comparison_string_pool.emplace(comparison_hash, StringNameEntry{ lower_case_str, comparison_hash });
        }
        display_string_pool.emplace(display_hash, StringNameEntry{ view, comparison_hash });
    }

    return { display_hash, comparison_hash };
}
