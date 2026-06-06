#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_REFLECT_ENUM(ETextureFormat)

SE_BEGIN_REFLECT(MipDescriptor, meta::Reflect)
    SE_REFLECT_PROPERTY(offset, meta::Reflect)
    SE_REFLECT_PROPERTY(size, meta::Reflect)
    SE_REFLECT_PROPERTY(width, meta::Reflect)
    SE_REFLECT_PROPERTY(height, meta::Reflect)
SE_END_REFLECT(MipDescriptor)

SE_BEGIN_REFLECT(Texture2D, meta::Reflect)
    SE_REFLECT_PROPERTY(width, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(height, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(format, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(generate_mips, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(mips, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(pixels, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT(Texture2D)
} // namespace se
