#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(StaticMesh)
    SE_REFLECT_PROPERTY(vertices, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(sections, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, ::se::meta::ReadOnly)
SE_END_REFLECT(StaticMesh)

SE_BEGIN_REFLECT(SkeletalMesh)
    SE_REFLECT_PROPERTY(vertices, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(skin_vertices, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(sections, ::se::meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, ::se::meta::ReadOnly)
SE_END_REFLECT(SkeletalMesh)
}  // namespace se::asset
