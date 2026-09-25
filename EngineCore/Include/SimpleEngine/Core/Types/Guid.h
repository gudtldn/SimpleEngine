#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
/**
 * 128비트 전역 고유 식별자(GUID/UUID) 클래스
 */
class SE_CORE_API Guid
{
public:
    Guid() noexcept = default;
    ~Guid() = default;

    Guid(const Guid& other) noexcept = default;
    Guid& operator=(const Guid& other) noexcept = default;
    Guid(Guid&& other) noexcept = default;
    Guid& operator=(Guid&& other) noexcept = default;

public:
    static const Guid None;

    [[nodiscard]] static Guid NewGuid();

    /**
     * 문자열로부터 생성합니다. 코드 안의 상수처럼 형식이 올바르다고 보장된 입력에만 씁니다.
     * @note 형식이 틀리면 상수식에서는 컴파일 에러, 런타임에서는 assert입니다. 데이터에서 읽은 문자열은 TryFromString을 씁니다.
     */
    [[nodiscard]] static constexpr Guid FromString(StringView view) noexcept
    {
        FixedArray<u8, 16> bytes{};
        const bool is_valid = ParseBytes(view, bytes);
        if consteval
        {
            if (!is_valid)
            {
                InvalidLiteralInConstantExpression();
            }
        }
        SE_ASSERT(is_valid, "Guid::FromString: invalid guid string. Use TryFromString for untrusted input.");
        return is_valid ? Guid{ bytes } : Guid{};
    }

    /**
     * 문자열로부터 생성을 시도합니다. 파일이나 사용자 입력처럼 형식을 보장할 수 없는 문자열에 씁니다.
     * @return 형식이 올바르지 않으면 NullOpt
     */
    [[nodiscard]] static constexpr Optional<Guid> TryFromString(StringView view) noexcept
    {
        FixedArray<u8, 16> bytes{};
        if (!ParseBytes(view, bytes))
        {
            return NullOpt;
        }
        return Guid{ bytes };
    }

public:
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] String ToString() const;

public:
    [[nodiscard]] explicit operator bool() const noexcept;
    [[nodiscard]] bool operator==(const Guid& other) const noexcept = default;

private:
    /** constexpr가 아니라서 상수식에서 불리면 컴파일 에러가 납니다. */
    static void InvalidLiteralInConstantExpression() noexcept {}

    /**
     * UUID 문자열을 16바이트로 파싱합니다.
     * @return 형식이 올바르지 않으면 false
     */
    static constexpr bool ParseBytes(StringView view, FixedArray<u8, 16>& out_bytes) noexcept
    {
        // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
        if (
            view.ByteLen() != 36
            || view[8] != '-' || view[13] != '-'
            || view[18] != '-' || view[23] != '-'
        )
        {
            return false;
        }

        constexpr auto hex_val = [](char c) noexcept -> int
        {
            if (c >= '0' && c <= '9') { return c - '0';      }
            if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
            if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
            return -1;
        };

        // 각 원소 = UUID 문자열에서 해당 바이트(2 hex char)의 시작 위치
        constexpr usize HEX_POS[16] = { 0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };

        for (usize i = 0; i < 16; ++i)
        {
            const int hi = hex_val(view[HEX_POS[i]]);
            const int lo = hex_val(view[HEX_POS[i] + 1]);
            if (hi < 0 || lo < 0)
            {
                return false;
            }
            out_bytes[i] = static_cast<u8>((hi << 4) | lo); // NOLINT(*-signed-bitwise)
        }

        return true;
    }

    constexpr explicit Guid(FixedArray<u8, 16> in_data) noexcept : data(in_data) {}

    FixedArray<u8, 16> data{};
};
} // namespace se

template <>
struct SE_CORE_API std::hash<se::Guid>
{
    usize operator()(const se::Guid& guid) const noexcept;
};

template <>
struct std::formatter<se::Guid, char> : std::formatter<se::String>
{
    auto format(const se::Guid& guid, std::format_context& ctx) const
    {
        return std::formatter<se::String>::format(guid.ToString(), ctx);
    }
};
