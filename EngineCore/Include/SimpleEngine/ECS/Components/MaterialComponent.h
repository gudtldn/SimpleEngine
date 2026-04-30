#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * Entity가 사용할 재질(Material) 리소스의 ID 목록을 지정하는 컴포넌트
 * material_override_ids[i]는 StaticMesh::sections[i]에 대응하는 머티리얼 오버라이드입니다.
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) MaterialHandleComponent
{
    SE_ANNOTATION(=meta::Property)
    Array<asset::AssetId> material_override_ids;

    // 프레임 중 MaterialInstance가 Evict되지 않도록 ref-count를 pin합니다.
    Array<asset::AssetHandle<asset::MaterialInstance>> instance_handles;
};
} // namespace se

SE_DECLARE_REFLECTION(se::MaterialHandleComponent)
