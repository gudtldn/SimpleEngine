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
    // Mesh Resource ID
    SE_ANNOTATION(=meta::Property)
    AssetId mesh_id;

    // RenderData Cache 추가
};
} // namespace se

SE_DECLARE_REFLECTION(se::StaticMeshComponent)
