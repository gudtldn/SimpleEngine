#include "SimpleEngine/ECS/Components/PersistentIdComponent.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Transient.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


// 월드 파일에는 컴포넌트가 아니라 엔티티의 id로 씀
SE_REFLECT_BEGIN(se::PersistentIdComponent, se::serde::Transient, se::ecs::Component)
    SE_FIELD(id)
SE_REFLECT_END()
