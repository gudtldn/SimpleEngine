#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT_V1(RGResourceBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGResourceBase)

SE_BEGIN_REFLECT_V1(RGTextureBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGTextureBase)

SE_BEGIN_REFLECT_V1(RGBufferBase, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGBufferBase)

SE_BEGIN_REFLECT_V1(RGTransientTexture, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGTransientTexture)

SE_BEGIN_REFLECT_V1(RGExternalTexture, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGExternalTexture)

SE_BEGIN_REFLECT_V1(RGTransientBuffer, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGTransientBuffer)

SE_BEGIN_REFLECT_V1(RGExternalBuffer, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT_V1(RGExternalBuffer)
} // namespace se
