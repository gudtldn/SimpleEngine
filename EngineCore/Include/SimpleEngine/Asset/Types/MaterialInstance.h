#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se::asset
{
// forward declaration
class Material;

/**
 * 머티리얼 인스턴스 에셋
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect) MaterialInstance : public AssetBase
{
    SE_CLASS(MaterialInstance, AssetBase)

public:
    // 부모 머티리얼 템플릿 ID
    SE_ANNOTATION(=meta::Property)
    AssetId parent_material_id;

    // 파라미터 데이터 블록 (Raw Bytes)
    SE_ANNOTATION(=meta::Property)
    Array<uint8> parameter_values;

    // 슬롯별 텍스처 오버라이드 맵
    SE_ANNOTATION(=meta::Property)
    HashMap<StringName, AssetId> texture_overrides;

public:
    /**
     * 슬롯 이름에 해당하는 텍스처 AssetId를 반환합니다.
     * 오버라이드가 없으면 parent Material의 default_texture_id를 반환합니다.
     * @param slot_name MaterialTextureSlot::name과 동일한 이름
     * @param parent 부모 Material 레퍼런스
     */
    [[nodiscard]] AssetId GetTextureOrDefault(StringName slot_name, const Material& parent) const;

    /** parameter_values의 참조를 반환합니다. */
    [[nodiscard]] const Array<uint8>& GetParameterBytes() const { return parameter_values; }
};
} // namespace se::asset