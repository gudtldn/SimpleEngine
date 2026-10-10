#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_ENUM_BEGIN(se::ESamplerType)
    SE_ENUM_VALUE(LinearRepeat)
    SE_ENUM_VALUE(LinearClamp)
    SE_ENUM_VALUE(PointRepeat)
    SE_ENUM_VALUE(PointClamp)
    SE_ENUM_VALUE(Max)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se::MaterialTextureSlot)
    SE_FIELD(name)
    SE_FIELD(fragment_slot)
    SE_FIELD(sampler)
    SE_FIELD(default_texture_id)
SE_REFLECT_END()
