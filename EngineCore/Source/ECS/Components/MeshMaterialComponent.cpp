#include "SimpleEngine/ECS/Components/MeshMaterialComponent.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for MeshMaterialComponent
SE_BEGIN_REFLECT_V1(MeshMaterialComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY_V1(material_overrides, meta::Reflect)
SE_END_REFLECT_V1(MeshMaterialComponent)
}
