#pragma once
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se
{
/**
 * Entity가 렌더링할 메시(Mesh) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=meta::Component) StaticMeshComponent
{
    // Mesh Resource ID
    SE_PROPERTY(=meta::Edit)
    asset::AssetId mesh_id;

    // RenderData Cache 추가
};
}  // namespace se
