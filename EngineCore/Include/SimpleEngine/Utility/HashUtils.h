#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
namespace detail
{
constexpr u64 FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
constexpr u64 FNV_PRIME = 0x100000001b3ULL;

/** FNV-1a 단일 바이트 누적 처리 */
constexpr void FNV1a_Byte(u64& hash, u8 byte) noexcept
{
    hash ^= byte;
    hash *= FNV_PRIME;
}

/** 64비트 정수(salt 등)를 리틀 엔디안 바이트 단위로 누적 */
constexpr void FNV1a_U64(u64& hash, u64 value) noexcept
{
    for (i32 i = 0; i < 8; ++i)
    {
        FNV1a_Byte(hash, static_cast<u8>(value >> (i * 8)));
    }
}
} // namespace detail

/**
 * 해시 관련 유틸리티 함수 모음
 * @see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
namespace hash
{
/** 원시 바이트 뷰에 대한 FNV-1a 해시 */
constexpr u64 FNV(ArrayView<const u8> bytes, u64 initial_hash = detail::FNV_OFFSET_BASIS) noexcept
{
    u64 hash = initial_hash;
    for (const u8 byte : bytes)
    {
        detail::FNV1a_Byte(hash, byte);
    }
    return hash;
}

/** 런타임 메모리 버퍼 FNV-1a 해시 */
inline u64 FNV(const void* data, usize size) noexcept
{
    return FNV(ArrayView<const u8>(static_cast<const u8*>(data), size));
}

/** 문자열에 대한 FNV-1a 64-bit 해시 */
constexpr u64 FNV(StringView view, u64 initial_hash = detail::FNV_OFFSET_BASIS) noexcept
{
    u64 hash = initial_hash;
    for (const char c : view)
    {
        detail::FNV1a_Byte(hash, static_cast<u8>(c));
    }
    return hash;
}

/** Salt를 포함한 문자열 FNV-1a 64-bit 해시 */
constexpr u64 FNVWithSalt(StringView view, u64 salt, u64 initial_hash = detail::FNV_OFFSET_BASIS) noexcept
{
    u64 hash = initial_hash;
    detail::FNV1a_U64(hash, salt);
    return FNV(view, hash);
}

/** 대소문자 무시 FNV-1a 64-bit 해시 (ASCII 전용) */
constexpr u64 FNVCaseInsensitive(StringView view, u64 initial_hash = detail::FNV_OFFSET_BASIS) noexcept
{
    u64 hash = initial_hash;
    for (const char c : view)
    {
        const u8 normalized = (c >= 'A' && c <= 'Z') ? static_cast<u8>(c | 0x20) : static_cast<u8>(c);
        detail::FNV1a_Byte(hash, normalized);
    }
    return hash;
}

template <typename... Ts>
constexpr void Combine(usize& seed, const Ts&... values)
{
    constexpr usize GOLDEN_RATIO = sizeof(usize) == 8
        ? static_cast<usize>(0x9e3779b97f4a7c15ULL)
        : static_cast<usize>(0x9e3779b9UL);

    const auto combine_one = [&]<typename T>(const T& v)
    {
        seed ^= std::hash<std::remove_cvref_t<T>>{}(v) + GOLDEN_RATIO + (seed << 6) + (seed >> 2);
    };
    (combine_one(values), ...);
}
} // namespace hash
} // namespace se
