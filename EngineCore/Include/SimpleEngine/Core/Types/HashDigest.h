#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#include <algorithm>
#include <cstring>


namespace se
{
/**
 * 고정 크기 Hash Digest
 *
 * SHA-256(32 bytes), xxHash128(16 bytes) 등의 해시 값을 인라인으로 저장합니다.
 * 힙 할당 없이 값 비교가 가능하며, 직렬화 시 hex 문자열로 변환합니다.
 *
 * @tparam N 해시 바이트 수 (예: 32 for SHA-256, 16 for xxHash128)
 */
template <usize N>
class HashDigest
{
    static_assert(N > 0 && N <= 64, "Hash digest size must be between 1 and 64 bytes");

public:
    static constexpr usize DIGEST_SIZE = N;

    constexpr HashDigest() = default;

public:
    /** 원시 바이트 배열로부터 생성 */
    static constexpr HashDigest FromRaw(const uint8 (&raw)[N])
    {
        HashDigest result;
        if consteval
        {
            for (usize i = 0; i < N; ++i)
            {
                result.data[i] = raw[i];
            }
        }
        else
        {
            std::memcpy(result.data.Data(), raw, N);
        }
        return result;
    }

    /** 원시 바이트 포인터로부터 생성 */
    static constexpr HashDigest FromRaw(const uint8* raw)
    {
        SE_ASSERT(raw != nullptr);

        HashDigest result;
        if consteval
        {
            for (usize i = 0; i < N; ++i)
            {
                result.data[i] = raw[i];
            }
        }
        else
        {
            std::memcpy(result.data.Data(), raw, N);
        }
        return result;
    }

    /** hex 문자열로부터 생성 (예: "db5c66474df3...") */
    static constexpr HashDigest FromHex(StringView hex)
    {
        HashDigest result;

        const usize expected_len = N * 2;
        if (hex.ByteLen() != expected_len)
        {
            return result; // zero-initialized
        }

        const char* src = hex.Data();
        for (usize i = 0; i < N; ++i)
        {
            SE_ASSERT(IsHexChar(src[i * 2]) && IsHexChar(src[(i * 2) + 1]), "FromHex: invalid hex character detected");
            result.data[i] = static_cast<uint8>(
                (HexCharToNibble(src[i * 2]) << 4) | HexCharToNibble(src[(i * 2) + 1])
            );
        }
        return result;
    }

    /** hex 문자열로 변환 */
    [[nodiscard]] String ToHex() const
    {
        static constexpr char HEX_CHARS[] = "0123456789abcdef";

        char buf[N * 2];
        for (usize i = 0; i < N; ++i)
        {
            buf[i * 2] = HEX_CHARS[data[i] >> 4];
            buf[(i * 2) + 1] = HEX_CHARS[data[i] & 0x0F];
        }
        return String{ buf, N * 2 };
    }

    /** Digest가 모두 0인지 확인 */
    [[nodiscard]] constexpr bool IsZero() const
    {
        for (usize i = 0; i < N; ++i)
        {
            if (data[i] != 0)
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] const uint8* Data() const { return data.Data(); }
    [[nodiscard]] uint8* Data() { return data.Data(); }

    [[nodiscard]] static constexpr usize Size() { return N; }

    [[nodiscard]] explicit constexpr operator bool() const { return !IsZero(); }
    [[nodiscard]] bool operator==(const HashDigest& other) const = default;

    /**
     * HashDigest 직렬화
     * Binary: raw bytes 직접 저장 (N bytes)
     * Text: hex 문자열로 변환 (N*2 chars)
     */
    friend void Serialize(Archive& ar, HashDigest& digest)
    {
        if (ar.IsBinary())
        {
            ar << BinaryBlob::FromBytes(digest.data.Data(), N);
        }
        else
        {
            String str;
            if (ar.IsSaving())
            {
                str = digest.ToHex();
            }

            ar << str;

            if (ar.IsLoading())
            {
                digest = HashDigest::FromHex(str);
            }
        }
    }

private:
    static constexpr bool IsHexChar(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static constexpr uint8 HexCharToNibble(char c)
    {
        if (c >= '0' && c <= '9') { return static_cast<uint8>(c - '0'); }
        if (c >= 'a' && c <= 'f') { return static_cast<uint8>(c - 'a' + 10); }
        if (c >= 'A' && c <= 'F') { return static_cast<uint8>(c - 'A' + 10); }
        return 0;
    }

    FixedArray<uint8, N> data{};
};

/** SHA-256 Hash Digest (32 bytes) */
using ContentHash = HashDigest<32>;
} // namespace se

namespace std
{
// NOLINTBEGIN(*-std-namespace-modification, *-dcl58-cpp)
template <usize N>
struct hash<se::HashDigest<N>>
{
    size_t operator()(const se::HashDigest<N>& digest) const noexcept
    {
        // Hash Digest는 이미 균일 분포이므로 앞 바이트를 그대로 사용
        size_t result = 0;
        std::memcpy(&result, digest.Data(), std::min(sizeof(size_t), N));
        return result;
    }
};

template <usize N>
struct formatter<se::HashDigest<N>, char> : std::formatter<se::String>
{
    auto format(const se::HashDigest<N>& digest, std::format_context& ctx) const
    {
        return std::formatter<se::String>::format(digest.ToHex(), ctx);
    }
};
// NOLINTEND(*-std-namespace-modification, *-dcl58-cpp)
} // namespace std
