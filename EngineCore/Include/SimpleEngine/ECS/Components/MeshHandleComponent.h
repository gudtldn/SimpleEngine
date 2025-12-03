#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Reflection/Annotations.h"


/**
 * Entity가 렌더링할 메시(Mesh) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=se::meta::Component) MeshHandleComponent
{
    // Mesh Resource ID
    SE_PROPERTY(=se::meta::Edit)
    uint32 mesh_id = 0;
};
