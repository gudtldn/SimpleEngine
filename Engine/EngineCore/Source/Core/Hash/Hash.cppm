export module SimpleEngine.Core.Hash;

import SimpleEngine.Platform.Types;
import std;

export namespace se::hash
{
/**
 * FNV-1a 해시 알고리즘을 사용한 문자열 해싱 함수
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
template <typename StringType>
constexpr uint64 FowlerNollVoHash(const StringType& in_str) noexcept
{
    using CharType = std::remove_cv_t<std::remove_pointer_t<decltype(std::data(in_str))>>;
    static_assert(sizeof(CharType) == 1, u8"Only 1-byte character types are supported");

    std::basic_string_view<CharType> sv = in_str;

    uint64 hash = 0xcbf29ce484222325ULL; // FNV_offset_basis
    for (const CharType c : sv)
    {
        const uint8 byte = static_cast<uint8>(c);
        hash ^= byte;
        hash *= 0x100000001b3ULL; // FNV_prime
    }
    return hash;
}
}
