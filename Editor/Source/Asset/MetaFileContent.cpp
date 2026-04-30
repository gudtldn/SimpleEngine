#include "SimpleEditor/Asset/MetaFileContent.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT(MetaFileContent, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(metadata, meta::Property)
    SE_REFLECT_PROPERTY(import_settings, meta::Property)
    SE_REFLECT_PROPERTY(processor_stack, meta::Property)
SE_END_REFLECT(MetaFileContent)
} // namespace se::editor
