#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::graphics
{
SE_BEGIN_REFLECT(IRGResource, meta::Internal)
SE_END_REFLECT(IRGResource)

SE_BEGIN_REFLECT(IRGTexture, meta::Internal)
SE_END_REFLECT(IRGTexture)

SE_BEGIN_REFLECT(IRGBuffer, meta::Internal)
SE_END_REFLECT(IRGBuffer)

SE_BEGIN_REFLECT(RGTransientTexture, meta::Internal)
SE_END_REFLECT(RGTransientTexture)

SE_BEGIN_REFLECT(RGExternalTexture, meta::Internal)
SE_END_REFLECT(RGExternalTexture)

SE_BEGIN_REFLECT(RGTransientBuffer, meta::Internal)
SE_END_REFLECT(RGTransientBuffer)

SE_BEGIN_REFLECT(RGExternalBuffer, meta::Internal)
SE_END_REFLECT(RGExternalBuffer)
}  // namespace se::graphics
