#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(StaticMesh)
    SE_REFLECT_PROPERTY(vertices, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::ReadOnly)
    SE_REFLECT_PROPERTY(sections, meta::ReadOnly)
SE_END_REFLECT(StaticMesh)

SE_BEGIN_REFLECT(SkeletalMesh)
    SE_REFLECT_PROPERTY(vertices, meta::ReadOnly)
    SE_REFLECT_PROPERTY(skin_vertices, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::ReadOnly)
    SE_REFLECT_PROPERTY(sections, meta::ReadOnly)
SE_END_REFLECT(SkeletalMesh)
}  // namespace se::asset
