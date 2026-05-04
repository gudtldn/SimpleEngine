#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Utility/Common.h"

#include <algorithm>
#include <cstring>


namespace se
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
    uint32 max_offset_plus_size = 0;
    for (const MaterialParameterDescriptor& desc : parameter_layout)
    {
        const uint32 end_pos = desc.offset + desc.GetSize();
        max_offset_plus_size = std::max(end_pos, max_offset_plus_size);
    }

    return static_cast<uint32>(AlignedSize<16>(max_offset_plus_size));
}

Optional<const MaterialParameterDescriptor&> Material::FindParameter(StringName name) const
{
    for (const MaterialParameterDescriptor& desc : parameter_layout)
    {
        if (desc.name == name)
        {
            return desc;
        }
    }
    return NullOpt;
}

Optional<const MaterialTextureSlot&> Material::FindTextureSlot(StringName name) const
{
    for (const MaterialTextureSlot& slot : texture_slots)
    {
        if (slot.name == name)
        {
            return slot;
        }
    }
    return NullOpt;
}

Material& Material::AddParameter(StringName name, EMaterialParamType type, Vector4f default_val)
{
    // 현재까지 쌓인 파라미터들의 끝 offset 계산
    uint32 offset = 0;
    for (const MaterialParameterDescriptor& desc : parameter_layout)
    {
        const uint32 end = desc.offset + desc.GetSize();
        offset = std::max(end, offset);
    }

    // 새 파라미터의 std140 alignment에 맞춰 offset 정렬
    const MaterialParameterDescriptor tmp = { .type = type };
    offset = static_cast<uint32>(AlignedSize(static_cast<usize>(offset), static_cast<usize>(tmp.GetAlignment())));

    parameter_layout.Push({
        .name = name,
        .type = type,
        .offset = offset,
        .default_value = default_val,
    });
    return *this;
}

void Material::FinalizeLayout()
{
    const uint32 block_size = ComputeParameterBlockSize();
    default_parameter_block.ResizeUninitialized(block_size);
    std::memset(default_parameter_block.Data(), 0, block_size);

    for (const MaterialParameterDescriptor& desc : parameter_layout)
    {
        std::memcpy(
            default_parameter_block.Data() + desc.offset,
            &desc.default_value,
            desc.GetSize()
        );
    }
}

const Array<uint8>& Material::GetDefaultParameterBlock() const
{
    return default_parameter_block;
}
} // namespace se
