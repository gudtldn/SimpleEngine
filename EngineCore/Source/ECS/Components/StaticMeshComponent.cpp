#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for StaticMeshComponent
SE_BEGIN_REFLECT_V1(StaticMeshComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(mesh_id, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(force_lod, meta::Reflect)
SE_END_REFLECT_V1(StaticMeshComponent)
}


SE_REFLECT_BEGIN(se::StaticMeshComponent)
    SE_FIELD(mesh_id)
    SE_FIELD(force_lod)
SE_REFLECT_END()
