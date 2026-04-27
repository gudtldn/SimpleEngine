#pragma once
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


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

    [[nodiscard]] static constexpr Guid FromString(StringView view) noexcept
    {
        // Format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 chars)
        if (
            view.ByteLen() != 36
            || view[8] != '-' || view[13] != '-'
            || view[18] != '-' || view[23] != '-'
        )
        {
            return {};
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

        FixedArray<uint8, 16> bytes{};
        for (usize i = 0; i < 16; ++i)
        {
            const int hi = hex_val(view[HEX_POS[i]]);
            const int lo = hex_val(view[HEX_POS[i] + 1]);
            if (hi < 0 || lo < 0)
            {
                return {};
            }
            bytes[i] = static_cast<uint8>((hi << 4) | lo);
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
    constexpr explicit Guid(FixedArray<uint8, 16> in_data) noexcept : data(in_data) {}

    FixedArray<uint8, 16> data{};
};
}  // namespace se

template <>
struct SE_CORE_API std::hash<se::Guid>
{
    size_t operator()(const se::Guid& guid) const noexcept;
};

template <>
struct std::formatter<se::Guid, char> : std::formatter<se::String>
{
    auto format(const se::Guid& guid, std::format_context& ctx) const
    {
        return std::formatter<se::String>::format(guid.ToString(), ctx);
    }
};
