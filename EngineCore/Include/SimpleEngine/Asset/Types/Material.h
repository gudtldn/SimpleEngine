#pragma once

#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"


namespace se
{
/**
 * 머티리얼 템플릿 에셋
 *
 * 셰이더 경로, 블렌드 모드, 파라미터 레이아웃, 텍스처 슬롯 정의를 가집니다.
 * PSO 키와 1:1 대응하므로, 인스턴스(MaterialInstance)가 달라도 같은 Material 이면 PSO를 재사용합니다.
 * 실제 파라미터 값과 텍스처 오버라이드는 MaterialInstance 에 저장합니다.
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect) Material : public AssetBase
{
    SE_CLASS(Material, AssetBase)

public:
    // 버텍스 셰이더 VPath
    SE_ANNOTATION(=meta::Property)
    VPath vertex_shader = "CoreShader://Default.vert";

    // 프래그먼트 셰이더 VPath
    SE_ANNOTATION(=meta::Property)
    VPath fragment_shader = "CoreShader://Default.frag";

    // 블렌드 모드
    SE_ANNOTATION(=meta::Property)
    EBlendMode blend_mode = EBlendMode::Opaque;

    // 셰이딩 모델 | TODO: 추후 PBR 전환 시 셰이더 퍼뮤테이션 키로 사용 예정
    SE_ANNOTATION(=meta::Property)
    EShadingModel shading_model = EShadingModel::Lit;

    // 양면 렌더링 여부
    SE_ANNOTATION(=meta::Property)
    bool two_sided = false;

    // 알파 컷오프 (Masked 전용)
    SE_ANNOTATION(=meta::Property)
    float alpha_cutoff = 0.5f;

    // 추후 셰이더 퍼뮤테이션 시스템 도입 시 사용할 키 (지금은 항상 0)
    SE_ANNOTATION(=meta::Property, =meta::Hidden)
    uint32 permutation_key = 0;

    // Fragment UBO 파라미터 레이아웃
    SE_ANNOTATION(=meta::Property)
    Array<MaterialParameterDescriptor> parameter_layout;

    // Fragment Texture 슬롯 정의
    SE_ANNOTATION(=meta::Property)
    Array<MaterialTextureSlot> texture_slots;

public:
    /**
     * 파라미터를 std140 규칙에 맞춰 자동 정렬 후 레이아웃에 추가합니다.
     * FinalizeLayout() 호출 전까지 여러 번 연속 호출 가능합니다.
     */
    Material& AddParameter(StringName name, EMaterialParamType type, Vector4f default_val = {});

    /**
     * 레이아웃을 확정하고 기본값 파라미터 블록을 생성합니다.
     * AddParameter() 호출이 모두 끝난 후 반드시 한 번 호출해야 합니다.
     */
    void FinalizeLayout();

    /** FinalizeLayout() 이후 기본값으로 채워진 파라미터 블록을 반환합니다. */
    [[nodiscard]] const Array<uint8>& GetDefaultParameterBlock() const;

    /** 전체 파라미터 블록의 바이트 크기 계산합니다. */
    [[nodiscard]] uint32 ComputeParameterBlockSize() const;

    /** 이름으로 파라미터 레이아웃을 검색합니다. 없으면 NullOpt를 반환합니다. */
    [[nodiscard]] Optional<const MaterialParameterDescriptor&> FindParameter(StringName name) const;

    /** 이름으로 텍스처 슬롯을 검색합니다. 없으면 NullOpt를 반환합니다. */
    [[nodiscard]] Optional<const MaterialTextureSlot&> FindTextureSlot(StringName name) const;

private:
    Array<uint8> default_parameter_block;
};
} // namespace se
