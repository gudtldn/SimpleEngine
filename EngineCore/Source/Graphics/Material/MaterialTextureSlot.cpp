#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::graphics
{
SE_REFLECT_ENUM(ESamplerType)

SE_BEGIN_REFLECT(MaterialTextureSlot, meta::Reflect)
    SE_REFLECT_PROPERTY(name, meta::Property)
    SE_REFLECT_PROPERTY(fragment_slot, meta::Property)
    SE_REFLECT_PROPERTY(sampler, meta::Property)
    SE_REFLECT_PROPERTY(default_texture_id, meta::Property)
SE_END_REFLECT(MaterialTextureSlot)
} // namespace se::graphics