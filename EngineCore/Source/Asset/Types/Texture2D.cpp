#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_REFLECT_ENUM(ETextureFormat)

SE_BEGIN_REFLECT(Texture2D, meta::Reflect)
    SE_REFLECT_PROPERTY(width, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(height, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(channels, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(format, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(is_srgb, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(generate_mips, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(pixels, meta::Property, meta::ReadOnly)
SE_END_REFLECT(Texture2D)
}  // namespace se::asset
