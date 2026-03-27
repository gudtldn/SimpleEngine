#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/HashUtils.h"

#include <functional>
#include <limits>


namespace se::graphics
{
namespace detail
{
/**
 * RenderGraph의 리소스를 나타내는 Handle 구조체
 */
template <typename Tag>
struct RGResourceHandleImpl
{
    /** resource_nodes 배열 내의 인덱스 */
    uint32 index = std::numeric_limits<uint32>::max();

    [[nodiscard]] constexpr bool IsValid() const { return index != std::numeric_limits<uint32>::max(); }
    [[nodiscard]] explicit constexpr operator bool() const { return IsValid(); }

    [[nodiscard]] constexpr bool operator==(const RGResourceHandleImpl&) const = default;
};
} // namespace detail

/** Render Graph 텍스처 리소스 핸들 */
using RGTextureHandle = detail::RGResourceHandleImpl<struct _RGTextureTag>;

/** Render Graph 버퍼 리소스 핸들 */
using RGBufferHandle = detail::RGResourceHandleImpl<struct _RGBufferTag>;
} // namespace se::graphics

template <typename Tag>
struct std::hash<se::graphics::detail::RGResourceHandleImpl<Tag>> // NOLINT(*-dcl58-cpp)
{
    size_t operator()(const se::graphics::detail::RGResourceHandleImpl<Tag>& handle) const noexcept
    {
        return std::hash<uint32>{}(handle.index);
    }
};
