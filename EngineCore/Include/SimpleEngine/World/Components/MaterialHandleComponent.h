#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Reflection/Annotations.h"


/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_TYPE_ANNOTATION(=se::meta::Component) MaterialHandleComponent
{
    SE_PROPERTY(=se::meta::Edit)
    uint32 material_id = 0;
};
