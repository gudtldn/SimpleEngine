#include "ECS/Components/ParentComponent.h"
#include "Core/Reflection/Reflect.h"


namespace se
{
// Reflection for ParentComponent
SE_BEGIN_REFLECT(ParentComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(parent, meta::Property)
SE_END_REFLECT(ParentComponent)
}
