#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


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


SE_REFLECT_BEGIN(se::RGResourceBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGTextureBase)
    SE_BASE(se::RGResourceBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGBufferBase)
    SE_BASE(se::RGResourceBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGTransientTexture)
    SE_BASE(se::RGTextureBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGExternalTexture)
    SE_BASE(se::RGTextureBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGTransientBuffer)
    SE_BASE(se::RGBufferBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGExternalBuffer)
    SE_BASE(se::RGBufferBase)
SE_REFLECT_END()
