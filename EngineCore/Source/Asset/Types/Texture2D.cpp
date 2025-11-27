#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(Texture2D)
    SE_REFLECT_PROPERTY(width, meta::ReadOnly)
    SE_REFLECT_PROPERTY(height, meta::ReadOnly)
    SE_REFLECT_PROPERTY(channels, meta::ReadOnly)
    SE_REFLECT_PROPERTY(format, meta::ReadOnly)
    SE_REFLECT_PROPERTY(is_srgb, meta::ReadOnly)
    SE_REFLECT_PROPERTY(generate_mips, meta::ReadOnly)
    SE_REFLECT_PROPERTY(pixels, meta::ReadOnly)
SE_END_REFLECT(Texture2D)
}
