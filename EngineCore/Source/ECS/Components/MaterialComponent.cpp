#include "ECS/Components/MaterialComponent.h"
#include "Core/Reflection/Reflect.h"


namespace se
{
// Reflection for MaterialHandleComponent
SE_BEGIN_REFLECT(MaterialHandleComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(material_id, meta::Property)
SE_END_REFLECT(MaterialHandleComponent)
}
