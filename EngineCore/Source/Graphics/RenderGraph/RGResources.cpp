#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT(RGResourceBase, meta::Internal)
SE_END_REFLECT(RGResourceBase)

SE_BEGIN_REFLECT(RGTextureBase, meta::Internal)
SE_END_REFLECT(RGTextureBase)

SE_BEGIN_REFLECT(RGBufferBase, meta::Internal)
SE_END_REFLECT(RGBufferBase)

SE_BEGIN_REFLECT(RGTransientTexture, meta::Internal)
SE_END_REFLECT(RGTransientTexture)

SE_BEGIN_REFLECT(RGExternalTexture, meta::Internal)
SE_END_REFLECT(RGExternalTexture)

SE_BEGIN_REFLECT(RGTransientBuffer, meta::Internal)
SE_END_REFLECT(RGTransientBuffer)

SE_BEGIN_REFLECT(RGExternalBuffer, meta::Internal)
SE_END_REFLECT(RGExternalBuffer)
} // namespace se
