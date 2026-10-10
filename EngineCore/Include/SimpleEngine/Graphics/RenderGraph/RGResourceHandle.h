#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"

#include <functional>
#include <limits>


namespace se
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
    u32 index = std::numeric_limits<u32>::max();

    [[nodiscard]] constexpr bool IsValid() const { return index != std::numeric_limits<u32>::max(); }
    [[nodiscard]] explicit constexpr operator bool() const { return IsValid(); }

    [[nodiscard]] constexpr bool operator==(const RGResourceHandleImpl&) const = default;
};
} // namespace detail

/** Render Graph 텍스처 리소스 핸들 */
using RGTextureHandle = detail::RGResourceHandleImpl<struct _RGTextureTag>;

/** Render Graph 버퍼 리소스 핸들 */
using RGBufferHandle = detail::RGResourceHandleImpl<struct _RGBufferTag>;

/** Render Graph 리소스 핸들 (필드는 index 하나) */
template <typename Tag>
struct Registrar<detail::RGResourceHandleImpl<Tag>>
{
    static void Fill(TypeInfo& info)
    {
        using T = detail::RGResourceHandleImpl<Tag>;

        info.size = sizeof(T);
        info.alignment = alignof(T);
        info.name = TypeNameOf<T>();

        auto& [bases, fields] = TypeRegistry::Get().EmplaceStructStorage(TypeId::Of<T>());
        fields.Push({
            .name = "index",
            .type = TypeId::Of<u32>(),
            .offset = ::se::detail::FieldOffsetOf<T>(&T::index),
            .annotations = {},
        });
        EnsureRegistered<u32>();
        info.shape = StructInfo{ .bases = bases, .fields = fields };
    }
};
} // namespace se

template <typename Tag>
struct std::hash<se::detail::RGResourceHandleImpl<Tag>> // NOLINT(*-dcl58-cpp)
{
    usize operator()(const se::detail::RGResourceHandleImpl<Tag>& handle) const noexcept
    {
        return std::hash<u32>{}(handle.index);
    }
};
