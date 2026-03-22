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

    /**
     * 리소스의 논리적 버전을 식별하는 값
     * @note 동일한 물리 리소스가 서로 다른 패스(Pass)에서 쓰기 작업 등으로 인해 상태가 변경될 때 이를 구분하기 위해 사용
     */
    uint32 version = 0;

    [[nodiscard]] constexpr bool IsValid() const { return index != std::numeric_limits<uint32>::max(); }
    [[nodiscard]] explicit constexpr operator bool() const { return IsValid(); }

    [[nodiscard]] constexpr bool operator==(const RGResourceHandleImpl&) const = default;
};
} // namespace detail

/** Render Graph 텍스처 리소스 핸들 */
using RGTextureHandle = detail::RGResourceHandleImpl<struct RGTextureTag>;

/** Render Graph 버퍼 리소스 핸들 */
using RGBufferHandle = detail::RGResourceHandleImpl<struct RGBufferTag>;
} // namespace se::graphics

template <typename Tag>
struct std::hash<se::graphics::detail::RGResourceHandleImpl<Tag>> // NOLINT(*-dcl58-cpp)
{
    size_t operator()(const se::graphics::detail::RGResourceHandleImpl<Tag>& handle) const noexcept
    {
        usize hash = 0;
        se::HashUtils::Combine(hash, handle.index, handle.version);
        return hash;
    }
};
