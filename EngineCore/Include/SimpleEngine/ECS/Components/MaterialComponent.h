#pragma once
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Meta/Annotations.h"


namespace se
{
/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=::se::meta::Component) MaterialHandleComponent
{
    SE_PROPERTY(=::se::meta::Edit)
    asset::AssetId material_id;
};
}  // namespace se
