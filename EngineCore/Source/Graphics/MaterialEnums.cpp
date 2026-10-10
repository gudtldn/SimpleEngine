#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


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

SE_REFLECT_ENUM_BEGIN(se::EMaterialFlag, se::display::BitFlags)
    SE_ENUM_VALUE(None)
    SE_ENUM_VALUE(AlphaTest)
SE_REFLECT_ENUM_END()
