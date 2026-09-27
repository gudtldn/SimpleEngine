#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"

#include "SimpleEngine/Core/Serialization/Legacy/MathSerialize.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"
#include "SimpleEngine/ECS/WorldFileSkip.h"


namespace se
{
SE_BEGIN_REFLECT_V1(GlobalTransformComponent, meta::Reflect, meta::Transient, meta::Component)
    SE_REFLECT_PROPERTY_V1(value, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT_V1(GlobalTransformComponent)
}


// TransformPropagation이 매 프레임 다시 계산하므로 월드 파일에 쓰지 않음
SE_REFLECT_BEGIN(se::GlobalTransformComponent, se::WorldFileSkip{})
SE_REFLECT_END()
