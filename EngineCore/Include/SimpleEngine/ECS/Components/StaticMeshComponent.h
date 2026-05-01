#pragma once
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"

namespace se
{
/**
 * Entity가 렌더링할 메시(Mesh) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) StaticMeshComponent
{
    /** 렌더링할 StaticMesh 에셋의 ID */
    SE_ANNOTATION(=meta::Property)
    AssetId mesh_id;

    /**
     * LOD 오버라이드
     * -1이면 카메라 거리에 따라 자동 계산(Auto LOD)되며, 0 이상의 값이면(예: 0, 1, 2) 거리에 상관없이 강제로 해당 LOD 레벨을 렌더링합니다.
     */
    SE_ANNOTATION(=meta::Property)
    int8 force_lod = -1;
};
} // namespace se

SE_DECLARE_REFLECTION(se::StaticMeshComponent)
