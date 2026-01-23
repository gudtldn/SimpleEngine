#include "Asset/ImportSettings/MeshImportSettings.h"
#include "Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(MeshImportSettings)
    SE_REFLECT_PROPERTY(combine_meshes, meta::Edit, meta::DisplayName("Combine Meshes"))
    SE_REFLECT_PROPERTY(apply_transform, meta::Edit, meta::DisplayName("Apply Transform"))
    SE_REFLECT_PROPERTY(global_scale, meta::Edit, meta::Range(0.1f, 1000.0f), meta::DisplayName("Global Scale"))
SE_END_REFLECT(MeshImportSettings)

void MeshImportSettings::Serialize(core::Archive& ar)
{
    ar << combine_meshes << apply_transform << global_scale;
}
}
