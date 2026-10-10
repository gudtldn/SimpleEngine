#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


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
