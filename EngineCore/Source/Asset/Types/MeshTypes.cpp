#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
SE_BEGIN_REFLECT(StaticMesh, meta::Reflect)
    SE_REFLECT_PROPERTY(vertices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(lods, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(default_materials, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, meta::Property, meta::ReadOnly)
SE_END_REFLECT(StaticMesh)

SE_BEGIN_REFLECT(SkeletalMesh, meta::Reflect)
    SE_REFLECT_PROPERTY(vertices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(skin_vertices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, meta::Property, meta::ReadOnly)
SE_END_REFLECT(SkeletalMesh)
} // namespace se
