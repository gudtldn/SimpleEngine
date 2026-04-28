#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) MaterialHandleComponent
{
    SE_ANNOTATION(=meta::Property)
    asset::AssetId material_id;

    // 프레임 중 MaterialInstance가 Evict되지 않도록 ref-count를 pin합니다.
    asset::AssetHandle<asset::MaterialInstance> instance_handle;
};
}  // namespace se

SE_DECLARE_REFLECTION(se::MaterialHandleComponent)
