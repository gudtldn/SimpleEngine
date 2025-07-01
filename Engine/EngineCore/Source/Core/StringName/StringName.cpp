module SimpleEngine.Core.StringName;

namespace
{
/**
 * FNV-1a 해시 알고리즘을 사용한 문자열 해싱 함수
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
constexpr uint64 ComputeHash(std::u8string_view in_str) noexcept
{
    uint64 hash = 0xcbf29ce484222325;
    for (const char8_t& c : in_str)
    {
        hash ^= c;
        hash *= 0x100000001b3;
    }
    return hash;
}
}

namespace se::core
{
StringName::StringName(std::u8string_view in_str)
{
    display_hash = ComputeHash(in_str);
    comparison_hash = ComputeHash(in_str); // TODO: Lowercase로 해야함
}

std::u8string StringName::ToString() const
{
    std::unreachable();
}
}
