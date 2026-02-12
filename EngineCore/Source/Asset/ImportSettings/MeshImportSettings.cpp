#include "Asset/ImportSettings/MeshImportSettings.h"
#include "Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(MeshImportSettings)
    SE_REFLECT_PROPERTY(combine_meshes, ::se::meta::Edit, ::se::meta::DisplayName("Combine Meshes"))
    SE_REFLECT_PROPERTY(apply_transform, ::se::meta::Edit, ::se::meta::DisplayName("Apply Transform"))
    SE_REFLECT_PROPERTY(global_scale, ::se::meta::Edit, ::se::meta::Range(0.01f, 1000.0f), ::se::meta::DisplayName("Global Scale"))
SE_END_REFLECT(MeshImportSettings)

void MeshImportSettings::Serialize(Archive& ar)
{
    ar << combine_meshes << apply_transform << global_scale;
}
}  // namespace se::asset
