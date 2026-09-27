#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"

#include "../../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se::editor
{
SE_BEGIN_REFLECT_V1(MeshImportSettings, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(combine_meshes, meta::Reflect, meta::DisplayName<"Combine Meshes">{})
    SE_REFLECT_PROPERTY_V1(apply_transform, meta::Reflect, meta::DisplayName<"Apply Transform">{})
    SE_REFLECT_PROPERTY_V1(global_scale, meta::Reflect, meta::Range(0.01f, 1000.0f), meta::DisplayName<"Global Scale">{})
SE_END_REFLECT_V1(MeshImportSettings)
} // namespace se::editor


SE_REFLECT_BEGIN(se::editor::MeshImportSettings)
    SE_BASE(se::editor::ImportSettingsBase)
    SE_FIELD(combine_meshes)
    SE_FIELD(apply_transform)
    SE_FIELD(global_scale)
SE_REFLECT_END()
