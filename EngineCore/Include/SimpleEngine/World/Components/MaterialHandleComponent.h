#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API MaterialHandleComponent
{
    uint32 material_id = 0;
};
