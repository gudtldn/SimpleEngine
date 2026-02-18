#pragma once
#include "SimpleEngine/Asset/AssetId.h"
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
};
}  // namespace se

SE_DECLARE_REFLECTION(se::MaterialHandleComponent)
