#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se
{
SE_REFLECT_ENUM_V1(ETextureFormat)

SE_BEGIN_REFLECT_V1(MipDescriptor, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(offset, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(size, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(width, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(height, meta::Reflect)
SE_END_REFLECT_V1(MipDescriptor)

SE_BEGIN_REFLECT_V1(Texture2D, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(width, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(height, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(format, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(generate_mips, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(mips, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(pixels, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT_V1(Texture2D)
} // namespace se
