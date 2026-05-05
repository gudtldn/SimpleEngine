#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"


namespace se::editor
{
/**
 * FBX/GLTF 임포트 과정에서 aiMaterial 하나를 MaterialInstance 에셋으로 변환하기 위한 Pipeline 노드
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) PipelineMaterialInstanceNode final : public PipelineBaseNode
{
    SE_CLASS(PipelineMaterialInstanceNode, PipelineBaseNode)

public:
    // 슬롯 이름 -> PipelineTextureNode UID ("BaseColor" -> tex_node.self_uid)
    HashMap<StringName, Guid> texture_node_refs;

    // 파라미터 이름 -> 오버라이드 값 (Float4 블록으로 저장; Float/Uint은 .x만 사용)
    HashMap<StringName, Vector4f> param_overrides;

    // 블렌드 모드 오버라이드
    Optional<EBlendMode> blend_mode_override;

    // 양면 렌더링 오버라이드
    Optional<bool> two_sided_override;

public:
    /** 텍스처 노드들을 의존성으로 추가하여 toposort가 텍스처 노드를 먼저 처리하도록 합니다. */
    virtual void GetFactoryDependencies(Array<Guid>& out_dependencies) const override;
};
} // namespace se::editor
