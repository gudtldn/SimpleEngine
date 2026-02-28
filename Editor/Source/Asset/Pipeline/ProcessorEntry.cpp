#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT(ProcessorEntry, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(processor_type, meta::Property)
    SE_REFLECT_PROPERTY(enabled, meta::Property)
SE_END_REFLECT(ProcessorEntry)
} // namespace se::editor
