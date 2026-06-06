#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT(RGResourceBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGResourceBase)

SE_BEGIN_REFLECT(RGTextureBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGTextureBase)

SE_BEGIN_REFLECT(RGBufferBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGBufferBase)

SE_BEGIN_REFLECT(RGTransientTexture, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGTransientTexture)

SE_BEGIN_REFLECT(RGExternalTexture, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGExternalTexture)

SE_BEGIN_REFLECT(RGTransientBuffer, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGTransientBuffer)

SE_BEGIN_REFLECT(RGExternalBuffer, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(RGExternalBuffer)
} // namespace se
