#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


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


SE_REFLECT_BEGIN(se::StaticMesh)
    SE_BASE(se::AssetBase)
    SE_FIELD(vertices)
    SE_FIELD(indices)
    SE_FIELD(lods)
    SE_FIELD(default_materials)
    SE_FIELD(bounds)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::SkeletalMesh)
    SE_BASE(se::AssetBase)
    SE_FIELD(vertices)
    SE_FIELD(skin_vertices)
    SE_FIELD(indices)
    SE_FIELD(bounds)
SE_REFLECT_END()
