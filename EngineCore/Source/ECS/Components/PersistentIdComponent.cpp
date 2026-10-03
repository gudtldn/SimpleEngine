#include "SimpleEngine/ECS/Components/PersistentIdComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Transient.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
SE_BEGIN_REFLECT_V1(PersistentIdComponent, meta::Reflect, meta::Transient, meta::Component)
    SE_REFLECT_PROPERTY_V1(id, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT_V1(PersistentIdComponent)
}


// 월드 파일에는 컴포넌트가 아니라 엔티티의 id로 씀
SE_REFLECT_BEGIN(se::PersistentIdComponent, se::Transient)
SE_REFLECT_END()
