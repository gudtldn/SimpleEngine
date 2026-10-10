#include "SimpleEngine/ECS/Components/ParentComponent.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::ParentComponent, se::ecs::Component)
    SE_FIELD(parent)
SE_REFLECT_END()
