#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"

#include <cstring>


namespace se
{
SE_BEGIN_REFLECT(MaterialInstance, meta::Reflect)
    SE_REFLECT_PROPERTY(parent_material_id, meta::Reflect)
    SE_REFLECT_PROPERTY(parameter_values, meta::Reflect)
    SE_REFLECT_PROPERTY(texture_overrides, meta::Reflect)
    SE_REFLECT_PROPERTY(blend_mode_override, meta::Reflect)
    SE_REFLECT_PROPERTY(two_sided_override, meta::Reflect)
SE_END_REFLECT(MaterialInstance)

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
