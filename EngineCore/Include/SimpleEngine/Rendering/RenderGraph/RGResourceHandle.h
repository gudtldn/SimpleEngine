#pragma once
#include <compare>
#include <limits>


namespace se::rendering
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
}

template<>
struct std::hash<se::rendering::RGResourceHandle>
{
    [[nodiscard]] size_t operator()(const se::rendering::RGResourceHandle& handle) const noexcept
    {
        return std::hash<usize>{}(handle.index);
    }
};
