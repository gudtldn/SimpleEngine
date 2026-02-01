#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(Texture2D)
    SE_REFLECT_PROPERTY(width, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(height, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(channels, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(format, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(is_srgb, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(generate_mips, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(pixels, ::se::meta::ReadOnly)
SE_END_REFLECT(Texture2D)
}  // namespace se::asset
