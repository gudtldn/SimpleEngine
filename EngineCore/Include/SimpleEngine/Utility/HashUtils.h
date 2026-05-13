#pragma once

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
namespace detail
{
constexpr u64 DEFAULT_FNV_HASH = 0xcbf29ce484222325ULL;
constexpr u64 FNV_PRIME = 0x100000001b3ULL;

/**
 * FNV-1a 해시 알고리즘을 사용한 문자열 해싱 함수
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
template <typename TransformFunc>
constexpr u64 FNV_Hash_Impl(StringView view, TransformFunc transform) noexcept
{
    u64 hash = DEFAULT_FNV_HASH; // FNV_offset_basis
    for (const StringView::CharType c : view)
    {
        hash ^= static_cast<u8>(transform(c));
        hash *= FNV_PRIME;
    }
    return hash;
}
} // namespace detail

/**
 * 해시 관련 유틸리티 함수 모음
 */
struct HashUtils
{
    HashUtils() = delete;

    /** 원시 바이트 범위에 대한 FNV-1a 해시 */
    static constexpr u64 FNV(const u8* data, usize size) noexcept
    {
        u64 hash = detail::DEFAULT_FNV_HASH;
        for (usize i = 0; i < size; ++i)
        {
            hash ^= static_cast<u64>(data[i]);
            hash *= detail::FNV_PRIME;
        }
        return hash;
    }

    static constexpr u64 FNV(StringView view) noexcept
    {
        return detail::FNV_Hash_Impl(view, [](auto c) { return c; });
    }

    static constexpr u64 FNVCaseInsensitive(StringView view) noexcept
    {
        return detail::FNV_Hash_Impl(view, [](auto c)
        {
            return ('A' <= c && c <= 'Z') ? c | 0x20 : c;
        });
    }

    template <typename... Ts>
    static void Combine(usize& seed, const Ts&... values)
    {
        const auto combine_one = [&]<typename T>(const T& v)
        {
            seed ^= std::hash<std::decay_t<T>>{}(v) + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
        };
        (combine_one(values), ...);
    }
};
} // namespace se
