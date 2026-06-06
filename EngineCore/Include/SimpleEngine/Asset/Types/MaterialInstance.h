#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"


namespace se
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
    SE_ANNOTATION(=meta::Reflect)
    AssetId parent_material_id;

    // 파라미터 데이터 블록 (Raw Bytes)
    SE_ANNOTATION(=meta::Reflect)
    Array<u8> parameter_values;

    // 슬롯별 텍스처 오버라이드 맵
    SE_ANNOTATION(=meta::Reflect)
    HashMap<StringName, AssetId> texture_overrides;

    // 블렌드 모드 오버라이드 (NullOpt이면 부모 머티리얼 값 사용)
    SE_ANNOTATION(=meta::Reflect)
    Optional<EBlendMode> blend_mode_override;

    // 양면 렌더링 오버라이드 (NullOpt이면 부모 머티리얼 값 사용)
    SE_ANNOTATION(=meta::Reflect)
    Optional<bool> two_sided_override;

public:
    /** 머티리얼의 실제 블렌드 모드를 반환합니다. (오버라이드 우선) */
    [[nodiscard]] EBlendMode GetBlendMode(const Material& parent) const;

    /** 머티리얼의 실제 양면 렌더링 여부를 반환합니다. (오버라이드 우선) */
    [[nodiscard]] bool IsTwoSided(const Material& parent) const;

public:
    /**
     * 슬롯 이름에 해당하는 텍스처 AssetId를 반환합니다.
     * 오버라이드가 없으면 parent Material의 default_texture_id를 반환합니다.
     * @param slot_name MaterialTextureSlot::name과 동일한 이름
     * @param parent 부모 Material 레퍼런스
     */
    [[nodiscard]] AssetId GetTextureOrDefault(StringName slot_name, const Material& parent) const;

    /**
     * 부모 Material의 기본값 파라미터 블록으로 parameter_values를 초기화합니다.
     * Material::FinalizeLayout()가 완료된 parent를 전달해야 합니다.
     */
    void InitializeFromParent(const Material& parent);

    /** parameter_values의 참조를 반환합니다. */
    [[nodiscard]] const Array<u8>& GetParameterBytes() const { return parameter_values; }
};
} // namespace se
