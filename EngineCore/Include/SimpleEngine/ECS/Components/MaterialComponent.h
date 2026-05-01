#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 * StaticMesh가 기본적으로 가진 default_materials 배열을 런타임 혹은 씬 레벨에서 덮어쓸(Override) 때 사용합니다.
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) MeshMaterialComponent
{
    /**
     * 메쉬의 각 섹션(Sub-mesh)에 매핑될 MaterialInstance 에셋 ID 배열
     * 인덱스 0번은 StaticMesh의 default_materials[0]을 덮어씁니다.
     *
     * @todo 나중에 오브젝트 개수가 많아져 ECS Cache Miss로 인한 성능 병목이 확인되면,
     *       동적 할당(Heap Allocation)을 제거하기 위해 FixedArray<AssetId, 8> 같은 인라인 배열 구조로 변경해야 함.
     */
    SE_ANNOTATION(=meta::Property)
    Array<AssetId> material_overrides;
};
} // namespace se

SE_DECLARE_REFLECTION(se::MeshMaterialComponent)
