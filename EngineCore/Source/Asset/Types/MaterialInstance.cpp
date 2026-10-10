#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"

#include <cstring>


namespace se
{
EBlendMode MaterialInstance::GetBlendMode(const Material& parent) const
{
    return blend_mode_override.ValueOr(parent.blend_mode);
}

bool MaterialInstance::IsTwoSided(const Material& parent) const
{
    return two_sided_override.ValueOr(parent.two_sided);
}

AssetId MaterialInstance::GetTextureOrDefault(StringName slot_name, const Material& parent) const
{
    // 인스턴스 오버라이드 우선
    if (const auto override_id = texture_overrides.Find(slot_name))
    {
        return *override_id;
    }

    // 부모 슬롯의 default_texture_id로 폴백
    if (const auto slot = parent.FindTextureSlot(slot_name))
    {
        return slot->default_texture_id;
    }

    return AssetId::invalid;
}

void MaterialInstance::InitializeFromParent(const Material& parent)
{
    const Array<u8>& block = parent.GetDefaultParameterBlock();
    parameter_values.ResizeUninitialized(block.Len());
    std::memcpy(parameter_values.Data(), block.Data(), block.Len());
}
} // namespace se


SE_REFLECT_BEGIN(se::MaterialInstance)
    SE_BASE(se::AssetBase)
    SE_FIELD(parent_material_id)
    SE_FIELD(parameter_values)
    SE_FIELD(texture_overrides)
    SE_FIELD(blend_mode_override)
    SE_FIELD(two_sided_override)
SE_REFLECT_END()
