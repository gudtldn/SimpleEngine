#pragma once
#include "SimpleEngine/Traits/TypeTraits.h"

#define SE_ENABLE_BITMASK_OPERATORS(enum_type) \
    constexpr ::se::BitFlags<enum_type> operator|(enum_type lhs, enum_type rhs) { return ::se::BitFlags<enum_type>(lhs) | rhs; } \
    constexpr ::se::BitFlags<enum_type> operator&(enum_type lhs, enum_type rhs) { return ::se::BitFlags<enum_type>(lhs) & rhs; } \
    constexpr ::se::BitFlags<enum_type> operator^(enum_type lhs, enum_type rhs) { return ::se::BitFlags<enum_type>(lhs) ^ rhs; } \
    constexpr ::se::BitFlags<enum_type> operator~(enum_type lhs) { return ~::se::BitFlags<enum_type>(lhs); }


namespace se
{
/**
 * Enum 타입을 비트마스크(Flags)처럼 안전하게 사용하기 위한 클래스
 * @tparam EnumType 비트마스크로 사용할 Enum 타입 (enum class 권장)
 */
template <traits::EnumType EnumType>
class BitFlags
{
public:
    using MaskType = std::underlying_type_t<EnumType>;

    constexpr BitFlags() = default;
    constexpr explicit BitFlags(EnumType in_bit) : mask_value(std::to_underlying(in_bit)) {}
    constexpr BitFlags(const BitFlags& in_other) = default;
    constexpr explicit BitFlags(MaskType in_value) : mask_value(in_value) {}

    constexpr BitFlags& operator=(const BitFlags& in_other) = default;

    [[nodiscard]] constexpr explicit operator bool() const { return mask_value != 0; }
    [[nodiscard]] constexpr explicit operator MaskType() const { return mask_value; }

public:
    [[nodiscard]] constexpr bool IsSet(EnumType bit) const
    {
        return (mask_value & std::to_underlying(bit)) == std::to_underlying(bit);
    }

    [[nodiscard]] constexpr bool IsAnySet(EnumType flags) const
    {
        return (mask_value & std::to_underlying(flags)) != 0;
    }

    [[nodiscard]] constexpr bool IsAnySet(BitFlags flags) const
    {
        return (mask_value & flags.mask_value) != 0;
    }

    constexpr void Set(EnumType bit)
    {
        mask_value |= std::to_underlying(bit);
    }

    constexpr void Unset(EnumType bit)
    {
        mask_value &= ~std::to_underlying(bit);
    }

    constexpr void Toggle(EnumType bit)
    {
        mask_value ^= std::to_underlying(bit);
    }

    constexpr void Clear()
    {
        mask_value = 0;
    }

    [[nodiscard]] constexpr MaskType GetValue() const { return mask_value; }

public:
    [[nodiscard]] constexpr BitFlags operator|(BitFlags other) const { return BitFlags(mask_value | other.mask_value); }
    [[nodiscard]] constexpr BitFlags operator&(BitFlags other) const { return BitFlags(mask_value & other.mask_value); }
    [[nodiscard]] constexpr BitFlags operator^(BitFlags other) const { return BitFlags(mask_value ^ other.mask_value); }

    constexpr BitFlags& operator|=(BitFlags other)
    {
        mask_value |= other.mask_value;
        return *this;
    }

    constexpr BitFlags& operator&=(BitFlags other)
    {
        mask_value &= other.mask_value;
        return *this;
    }

    constexpr BitFlags& operator^=(BitFlags other)
    {
        mask_value ^= other.mask_value;
        return *this;
    }

    [[nodiscard]] constexpr BitFlags operator|(EnumType bit) const { return BitFlags(mask_value | std::to_underlying(bit)); }
    [[nodiscard]] constexpr BitFlags operator&(EnumType bit) const { return BitFlags(mask_value & std::to_underlying(bit)); }
    [[nodiscard]] constexpr BitFlags operator^(EnumType bit) const { return BitFlags(mask_value ^ std::to_underlying(bit)); }

    constexpr BitFlags& operator|=(EnumType bit)
    {
        mask_value |= std::to_underlying(bit);
        return *this;
    }

    constexpr BitFlags& operator&=(EnumType bit)
    {
        mask_value &= std::to_underlying(bit);
        return *this;
    }

    constexpr BitFlags& operator^=(EnumType bit)
    {
        mask_value ^= std::to_underlying(bit);
        return *this;
    }

    [[nodiscard]] constexpr BitFlags operator~() const
    {
        return BitFlags(~mask_value);
    }

    [[nodiscard]] constexpr bool operator==(BitFlags other) const noexcept { return mask_value == other.mask_value; }
    [[nodiscard]] constexpr bool operator==(EnumType bit) const noexcept { return mask_value == std::to_underlying(bit); }

private:
    MaskType mask_value = 0;
};
}
