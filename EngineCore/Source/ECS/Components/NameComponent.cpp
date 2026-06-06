#include "SimpleEngine/ECS/Components/NameComponent.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
// Reflection for NameComponent
SE_BEGIN_REFLECT(NameComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(name, meta::Reflect)
SE_END_REFLECT(NameComponent)
}
