#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(StaticMesh, meta::Reflect)
    SE_REFLECT_PROPERTY(vertices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, meta::Property, meta::ReadOnly)
SE_END_REFLECT(StaticMesh)

SE_BEGIN_REFLECT(SkeletalMesh, meta::Reflect)
    SE_REFLECT_PROPERTY(vertices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(skin_vertices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(indices, meta::Property, meta::ReadOnly)
    SE_REFLECT_PROPERTY(bounds, meta::Property, meta::ReadOnly)
SE_END_REFLECT(SkeletalMesh)
} // namespace se::asset
