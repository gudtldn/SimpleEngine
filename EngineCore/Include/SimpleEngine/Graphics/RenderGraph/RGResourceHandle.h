#pragma once
#include <compare>
#include <limits>


namespace se::graphics
{
/**
 * Render Graph 내의 리소스를 가리키는 핸들
 */
struct RGResourceHandle
{
    usize index = std::numeric_limits<usize>::max();

    [[nodiscard]] constexpr bool operator==(const RGResourceHandle&) const = default;
    [[nodiscard]] constexpr auto operator<=>(const RGResourceHandle&) const = default;

    [[nodiscard]] constexpr bool IsValid() const { return index != std::numeric_limits<usize>::max(); }
    [[nodiscard]] explicit constexpr operator bool() const { return IsValid(); }
};
}  // namespace se::graphics

template<>
struct std::hash<se::graphics::RGResourceHandle>
{
    [[nodiscard]] size_t operator()(const se::graphics::RGResourceHandle& handle) const noexcept
    {
        return std::hash<usize>{}(handle.index);
    }
};
