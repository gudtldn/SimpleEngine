#include "ECS/Components/MaterialComponent.h"
#include "Reflection/Reflect.h"

using namespace se;


// Reflection for MaterialHandleComponent
SE_BEGIN_REFLECT(MaterialHandleComponent, ::se::meta::Component)
    SE_REFLECT_PROPERTY(material_id, ::se::meta::Edit)
SE_END_REFLECT(MaterialHandleComponent)
