#include "ECS/Components/StaticMeshComponent.h"
#include "Core/Reflection/Reflect.h"


namespace se
{
// Reflection for StaticMeshComponent
SE_BEGIN_REFLECT(StaticMeshComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(mesh_id, meta::Property)
SE_END_REFLECT(StaticMeshComponent)
}
