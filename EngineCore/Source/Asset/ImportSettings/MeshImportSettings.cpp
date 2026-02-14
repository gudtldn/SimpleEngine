#include "SimpleEngine/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::asset
{
SE_BEGIN_REFLECT(MeshImportSettings, meta::Reflect)
    SE_REFLECT_PROPERTY(combine_meshes, meta::Property, meta::DisplayName<"Combine Meshes">{})
    SE_REFLECT_PROPERTY(apply_transform, meta::Property, meta::DisplayName<"Apply Transform">{})
    SE_REFLECT_PROPERTY(global_scale, meta::Property, meta::Range(0.01f, 1000.0f), meta::DisplayName<"Global Scale">{})
SE_END_REFLECT(MeshImportSettings)

void MeshImportSettings::Serialize(Archive& ar)
{
    ar << combine_meshes << apply_transform << global_scale;
}
}  // namespace se::asset
