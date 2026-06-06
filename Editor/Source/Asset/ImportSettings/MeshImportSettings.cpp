#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT(MeshImportSettings, meta::Reflect)
    SE_REFLECT_PROPERTY(combine_meshes, meta::Reflect, meta::DisplayName<"Combine Meshes">{})
    SE_REFLECT_PROPERTY(apply_transform, meta::Reflect, meta::DisplayName<"Apply Transform">{})
    SE_REFLECT_PROPERTY(global_scale, meta::Reflect, meta::Range(0.01f, 1000.0f), meta::DisplayName<"Global Scale">{})
SE_END_REFLECT(MeshImportSettings)
} // namespace se::editor
