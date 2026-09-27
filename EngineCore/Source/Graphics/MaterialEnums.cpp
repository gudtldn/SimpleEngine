#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se
{
SE_REFLECT_ENUM_V1(EBlendMode)
SE_REFLECT_ENUM_V1(EShadingModel)
SE_REFLECT_ENUM_V1(EMaterialFlag)
} // namespace se


SE_REFLECT_ENUM_BEGIN(se::EBlendMode)
    SE_ENUM_VALUE(Opaque)
    SE_ENUM_VALUE(Masked)
    SE_ENUM_VALUE(Translucent)
    SE_ENUM_VALUE(Additive)
    SE_ENUM_VALUE(Modulate)
SE_REFLECT_ENUM_END()

SE_REFLECT_ENUM_BEGIN(se::EShadingModel)
    SE_ENUM_VALUE(Lit)
    SE_ENUM_VALUE(Unlit)
    SE_ENUM_VALUE(Subsurface)
    SE_ENUM_VALUE(ClearCoat)
SE_REFLECT_ENUM_END()
