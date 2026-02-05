#pragma once
#include <concepts>

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
namespace detail
{
constexpr uint64 DefaultFNVHash = 0xcbf29ce484222325ULL;
constexpr uint64 FNV_Prime = 0x100000001b3ULL;

/**
 * FNV-1a 해시 알고리즘을 사용한 문자열 해싱 함수
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
template <typename TransformFunc>
constexpr uint64 FNV_Hash_Impl(StringView view, TransformFunc transform) noexcept
{
    uint64 hash = DefaultFNVHash; // FNV_offset_basis
    for (const StringView::CharType c : view)
    {
        hash ^= static_cast<uint8>(transform(c));
        hash *= FNV_Prime;
    }
    return hash;
}
}  // namespace detail

/**
 * 해시 관련 유틸리티 함수 모음
 */
struct HashUtils
{
    HashUtils() = delete;

    static constexpr uint64 FNV(StringView view) noexcept
    {
        return detail::FNV_Hash_Impl(view, [](auto c) { return c; });
    }

    static constexpr uint64 FNVCaseInsensitive(StringView view) noexcept
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
}  // namespace se
