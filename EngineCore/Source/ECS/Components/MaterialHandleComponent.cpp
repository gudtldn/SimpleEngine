#include "ECS/Components/MaterialHandleComponent.h"
#include "Reflection/Reflect.h"

using namespace se;


// Reflection for MaterialHandleComponent
SE_BEGIN_REFLECT(MaterialHandleComponent, meta::Component)
    SE_REFLECT_PROPERTY(material_id, meta::Edit)
SE_END_REFLECT(MaterialHandleComponent)
