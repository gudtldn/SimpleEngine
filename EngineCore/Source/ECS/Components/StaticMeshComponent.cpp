#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for StaticMeshComponent
SE_BEGIN_REFLECT_V1(StaticMeshComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(mesh_id, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(force_lod, meta::Reflect)
SE_END_REFLECT_V1(StaticMeshComponent)
}
