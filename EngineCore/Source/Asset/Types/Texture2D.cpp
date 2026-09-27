#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


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


SE_REFLECT_ENUM_BEGIN(se::ETextureFormat)
    SE_ENUM_VALUE(None)
    SE_ENUM_VALUE(R8_UNORM)
    SE_ENUM_VALUE(R8G8_UNORM)
    SE_ENUM_VALUE(R8G8B8A8_UNORM)
    SE_ENUM_VALUE(R8G8B8A8_UNORM_SRGB)
    SE_ENUM_VALUE(R16G16_FLOAT)
    SE_ENUM_VALUE(R16G16B16A16_FLOAT)
    SE_ENUM_VALUE(R11G11B10_UFLOAT)
    SE_ENUM_VALUE(BC1_UNORM)
    SE_ENUM_VALUE(BC3_UNORM)
    SE_ENUM_VALUE(BC4_UNORM)
    SE_ENUM_VALUE(BC5_UNORM)
    SE_ENUM_VALUE(BC7_UNORM)
    SE_ENUM_VALUE(BC1_UNORM_SRGB)
    SE_ENUM_VALUE(BC3_UNORM_SRGB)
    SE_ENUM_VALUE(BC7_UNORM_SRGB)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se::MipDescriptor)
    SE_FIELD(offset)
    SE_FIELD(size)
    SE_FIELD(width)
    SE_FIELD(height)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Texture2D)
    SE_BASE(se::AssetBase)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(format)
    SE_FIELD(generate_mips)
    SE_FIELD(mips)
    SE_FIELD(pixels)
SE_REFLECT_END()
