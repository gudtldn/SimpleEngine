#include "SimpleEngine/ECS/Components/MaterialComponent.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
// Reflection for MeshMaterialComponent
SE_BEGIN_REFLECT(MeshMaterialComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(material_id, meta::Property)
SE_END_REFLECT(MeshMaterialComponent)
}
