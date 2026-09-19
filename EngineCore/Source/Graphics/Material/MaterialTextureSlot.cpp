#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se
{
SE_REFLECT_ENUM_V1(ESamplerType)

SE_BEGIN_REFLECT_V1(MaterialTextureSlot, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(name, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(fragment_slot, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(sampler, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(default_texture_id, meta::Reflect)
SE_END_REFLECT_V1(MaterialTextureSlot)
} // namespace se
