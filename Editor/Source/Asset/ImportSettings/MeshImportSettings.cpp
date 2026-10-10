#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::editor::MeshImportSettings)
    SE_BASE(se::editor::ImportSettingsBase)
    SE_FIELD(combine_meshes)
    SE_FIELD(apply_transform)
    SE_FIELD(global_scale)
SE_REFLECT_END()
