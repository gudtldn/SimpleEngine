#include "SimpleEditor/Asset/MetaFileContent.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se::editor
{
SE_BEGIN_REFLECT_V1(MetaFileContent, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(metadata, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(import_settings, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(processor_stack, meta::Reflect)
SE_END_REFLECT_V1(MetaFileContent)
} // namespace se::editor


SE_REFLECT_BEGIN(se::editor::MetaFileContent)
    SE_FIELD(metadata)
    SE_FIELD(import_settings)
    SE_FIELD(processor_stack)
SE_REFLECT_END()
