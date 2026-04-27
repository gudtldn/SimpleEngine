#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(Material, meta::Reflect)
    SE_REFLECT_PROPERTY(vertex_shader, meta::Property)
    SE_REFLECT_PROPERTY(fragment_shader, meta::Property)
    SE_REFLECT_PROPERTY(blend_mode, meta::Property)
    SE_REFLECT_PROPERTY(shading_model, meta::Property)
    SE_REFLECT_PROPERTY(two_sided, meta::Property)
    SE_REFLECT_PROPERTY(alpha_cutoff, meta::Property)
    SE_REFLECT_PROPERTY(permutation_key, meta::Property, meta::Hidden)
    SE_REFLECT_PROPERTY(parameter_layout, meta::Property)
    SE_REFLECT_PROPERTY(texture_slots, meta::Property)
SE_END_REFLECT(Material)

uint32 Material::ComputeParameterBlockSize() const
{
    uint32 total = 0;
    for (const graphics::MaterialParameterDescriptor& desc : parameter_layout)
    {
        total += desc.GetSize();
    }
    return total;
}

Optional<const graphics::MaterialParameterDescriptor&> Material::FindParameter(StringName name) const
{
    for (const graphics::MaterialParameterDescriptor& desc : parameter_layout)
    {
        if (desc.name == name)
        {
            return desc;
        }
    }
    return NullOpt;
}

Optional<const graphics::MaterialTextureSlot&> Material::FindTextureSlot(StringName name) const
{
    for (const graphics::MaterialTextureSlot& slot : texture_slots)
    {
        if (slot.name == name)
        {
            return slot;
        }
    }
    return NullOpt;
}
} // namespace se::asset