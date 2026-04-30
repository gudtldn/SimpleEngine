#include "SimpleEngine/ECS/Components/MaterialComponent.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
// Reflection for MaterialHandleComponent
SE_BEGIN_REFLECT(MaterialHandleComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(material_override_ids, meta::Property)
SE_END_REFLECT(MaterialHandleComponent)
}
