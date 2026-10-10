#include "SimpleEngine/ECS/Components/MeshMaterialComponent.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::MeshMaterialComponent, se::ecs::Component)
    SE_FIELD(material_overrides)
SE_REFLECT_END()
