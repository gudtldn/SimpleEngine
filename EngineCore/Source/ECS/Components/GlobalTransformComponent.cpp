#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"

#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Transient.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


// TransformPropagation이 매 프레임 다시 계산하므로 월드 파일에 쓰지 않음
SE_REFLECT_BEGIN(se::GlobalTransformComponent, se::serde::Transient, se::ecs::Component)
    SE_FIELD(value)
SE_REFLECT_END()
