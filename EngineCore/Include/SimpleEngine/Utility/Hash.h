#pragma once
#include <string_view>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::utility
{
/**
 * FNV-1a 해시 알고리즘을 사용한 문자열 해싱 함수
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
template <typename StringType>
    requires requires { typename StringType::value_type; }
constexpr uint64 FNV_Hash(const StringType& in_str) noexcept
{
    using CharType = StringType::value_type;
    static_assert(sizeof(CharType) == 1, "Only 1-byte character types are supported");

    std::basic_string_view<CharType> sv{ in_str };

    uint64 hash = 0xcbf29ce484222325ULL; // FNV_offset_basis
    for (const CharType c : sv)
    {
        const uint8 byte = static_cast<uint8>(c);
        hash ^= byte;
        hash *= 0x100000001b3ULL; // FNV_prime
    }
    return hash;
}

template <typename T>
void HashCombine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename... Ts>
void HashCombine(size_t& seed, const Ts&... values)
{
    (HashCombine(seed, values), ...);
}
}
