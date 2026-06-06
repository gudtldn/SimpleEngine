#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
// Reflection for StaticMeshComponent
SE_BEGIN_REFLECT(StaticMeshComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(mesh_id, meta::Reflect)
    SE_REFLECT_PROPERTY(force_lod, meta::Reflect)
SE_END_REFLECT(StaticMeshComponent)
}
