#include "SimpleEngine/ECS/Components/ParentComponent.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for ParentComponent
SE_BEGIN_REFLECT(ParentComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(parent, meta::Reflect)
SE_END_REFLECT(ParentComponent)
}
