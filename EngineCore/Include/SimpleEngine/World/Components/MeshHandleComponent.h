#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


/**
 * Entity가 렌더링할 메시(Mesh) 리소스의 ID를 지정하는 컴포넌트
 */
struct MeshHandleComponent
{
    // Mesh Resource ID
    uint32 mesh_id = 0;
};
