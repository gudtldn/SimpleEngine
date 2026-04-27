#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(MaterialInstance, meta::Reflect)
    SE_REFLECT_PROPERTY(parent_material_id, meta::Property)
    SE_REFLECT_PROPERTY(parameter_values, meta::Property)
    SE_REFLECT_PROPERTY(texture_overrides, meta::Property)
SE_END_REFLECT(MaterialInstance)

AssetId MaterialInstance::GetTextureOrDefault(StringName slot_name, const Material& parent) const
{
    // 인스턴스 오버라이드 우선
    if (const auto override_id = texture_overrides.Find(slot_name))
    {
        return *override_id;
    }

    // 부모 슬롯의 default_texture_id 로 폴백
    if (const auto slot = parent.FindTextureSlot(slot_name))
    {
        return slot->default_texture_id;
    }

    return AssetId::Invalid;
}
} // namespace se::asset