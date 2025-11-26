#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(Texture2D)
    SE_REFLECT_PROPERTY(width)
    SE_REFLECT_PROPERTY(height)
    SE_REFLECT_PROPERTY(channels)
    SE_REFLECT_PROPERTY(format)
    SE_REFLECT_PROPERTY(is_srgb)
    SE_REFLECT_PROPERTY(generate_mips)
    SE_REFLECT_PROPERTY(pixels)
SE_END_REFLECT(Texture2D)
}
