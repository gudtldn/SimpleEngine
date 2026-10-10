#include "SimpleEngine/ECS/Components/NameComponent.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::NameComponent, se::ecs::Component)
    SE_FIELD(name)
SE_REFLECT_END()
