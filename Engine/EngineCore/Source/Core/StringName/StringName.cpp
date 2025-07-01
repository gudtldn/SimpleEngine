module SimpleEngine.Core.StringName;

import SimpleEngine.Utils;


namespace
{
/**
 * FNV-1a 해시 알고리즘을 사용한 문자열 해싱 함수
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
constexpr uint64 ComputeHash(std::u8string_view in_str) noexcept
{
    uint64 hash = 0xcbf29ce484222325ULL;
    for (const char8_t c : in_str)
    {
        hash ^= c;
        hash *= 0x100000001b3ULL;
    }
    return hash;
}
}

namespace se::core
{
StringName::StringName(const char8* in_str)
    : StringName(std::u8string_view(in_str))
{
}

StringName::StringName(std::u8string_view in_str)
{
    // TODO: StringNamePool에 등록해야함
    display_hash = ComputeHash(in_str);
    comparison_hash = ComputeHash(string_utils::ToU8LowerCase(in_str));
}

std::u8string StringName::ToString() const
{
    // TODO: Implements this
    std::unreachable();
}
}
