#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::StaticMeshComponent, se::ecs::Component)
    SE_FIELD(mesh_id)
    SE_FIELD(force_lod)
SE_REFLECT_END()
