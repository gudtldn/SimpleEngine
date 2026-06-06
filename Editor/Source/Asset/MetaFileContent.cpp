#include "SimpleEditor/Asset/MetaFileContent.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT(MetaFileContent, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(metadata, meta::Reflect)
    SE_REFLECT_PROPERTY(import_settings, meta::Reflect)
    SE_REFLECT_PROPERTY(processor_stack, meta::Reflect)
SE_END_REFLECT(MetaFileContent)
} // namespace se::editor
