#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT(StaticMesh, meta::Reflect)
    SE_REFLECT_PROPERTY(vertices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(lods, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(default_materials, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT(StaticMesh)

SE_BEGIN_REFLECT(SkeletalMesh, meta::Reflect)
    SE_REFLECT_PROPERTY(vertices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(skin_vertices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::Reflect, meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, meta::Reflect, meta::ReadOnly)
SE_END_REFLECT(SkeletalMesh)
} // namespace se
