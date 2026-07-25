#include "SimpleEngine/ECS/Components/MeshMaterialComponent.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// Reflection for MeshMaterialComponent
SE_BEGIN_REFLECT(MeshMaterialComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(material_overrides, meta::Reflect)
SE_END_REFLECT(MeshMaterialComponent)
}
