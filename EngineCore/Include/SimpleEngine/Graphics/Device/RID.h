#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <functional>
#include <limits>


namespace se::graphics
{
/**
 * GPU 리소스를 가리키는 불투명 핸들 (Render resource ID)
 *
 * 32-bit index + 32-bit generation 으로 구성된 generational handle
 * SlotMap<T>과 함께 사용하여 use-after-free를 방지합니다.
 */
struct RID
{
    static constexpr uint32 INVALID_INDEX = std::numeric_limits<uint32>::max();
    static constexpr uint32 INVALID_GENERATION = 0;

    uint32 index = INVALID_INDEX;
    uint32 generation = INVALID_GENERATION;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return index != INVALID_INDEX && generation != INVALID_GENERATION;
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept { return IsValid(); }

    [[nodiscard]] constexpr bool operator==(const RID&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const RID&) const noexcept = default;

    /** 64비트 패킹 값 반환 (주로 디버그 출력용) */
    [[nodiscard]] constexpr uint64 ToU64() const noexcept
    {
        return (static_cast<uint64>(generation) << 32) | static_cast<uint64>(index);
    }
};
} // namespace se::graphics


template<>
struct std::hash<se::graphics::RID>
{
    [[nodiscard]] size_t operator()(const se::graphics::RID& rid) const noexcept
    {
        return std::hash<uint64>{}(rid.ToU64());
    }
};
