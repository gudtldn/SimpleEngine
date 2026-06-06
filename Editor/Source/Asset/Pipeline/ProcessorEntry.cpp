#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT(ProcessorEntry, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(processor_type, meta::Reflect)
    SE_REFLECT_PROPERTY(enabled, meta::Reflect)
SE_END_REFLECT(ProcessorEntry)
} // namespace se::editor
