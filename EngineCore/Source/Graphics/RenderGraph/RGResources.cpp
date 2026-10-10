#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::RGResourceBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGTextureBase)
    SE_BASE(se::RGResourceBase)
    SE_FIELD(actual_texture)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGBufferBase)
    SE_BASE(se::RGResourceBase)
    SE_FIELD(actual_buffer)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGTransientTexture)
    SE_BASE(se::RGTextureBase)
    SE_FIELD(description)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGExternalTexture)
    SE_BASE(se::RGTextureBase)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGTransientBuffer)
    SE_BASE(se::RGBufferBase)
    SE_FIELD(description)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RGExternalBuffer)
    SE_BASE(se::RGBufferBase)
SE_REFLECT_END()
