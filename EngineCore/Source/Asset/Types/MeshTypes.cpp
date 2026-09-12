#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT_V1(StaticMesh, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(vertices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(indices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(lods, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(default_materials, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(bounds, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT_V1(StaticMesh)

SE_BEGIN_REFLECT_V1(SkeletalMesh, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(vertices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(skin_vertices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(indices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY_V1(bounds, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT_V1(SkeletalMesh)
} // namespace se
