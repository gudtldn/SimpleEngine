#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::graphics
{
SE_BEGIN_REFLECT(IRGResource)
SE_END_REFLECT(IRGResource)

SE_BEGIN_REFLECT(IRGTexture)
SE_END_REFLECT(IRGTexture)

SE_BEGIN_REFLECT(IRGBuffer)
SE_END_REFLECT(IRGBuffer)

SE_BEGIN_REFLECT(RGTransientTexture)
SE_END_REFLECT(RGTransientTexture)

SE_BEGIN_REFLECT(RGExternalTexture)
SE_END_REFLECT(RGExternalTexture)

SE_BEGIN_REFLECT(RGTransientBuffer)
SE_END_REFLECT(RGTransientBuffer)

SE_BEGIN_REFLECT(RGExternalBuffer)
SE_END_REFLECT(RGExternalBuffer)
}  // namespace se::graphics
