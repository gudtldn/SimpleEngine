#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Reflection/Reflect.h"


/**
 * Entity가 렌더링할 메시(Mesh) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API MeshHandleComponent
{
    SE_REFLECTABLE()

public:
    // Mesh Resource ID
    SE_PROPERTY()
    uint32 mesh_id = 0;
};
