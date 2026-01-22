#include "ECS/Components/StaticMeshComponent.h"
#include "Reflection/Reflect.h"

using namespace se;


// Reflection for StaticMeshComponent
SE_BEGIN_REFLECT(StaticMeshComponent, meta::Component)
    SE_REFLECT_PROPERTY(mesh_id, meta::Edit)
SE_END_REFLECT(StaticMeshComponent)
