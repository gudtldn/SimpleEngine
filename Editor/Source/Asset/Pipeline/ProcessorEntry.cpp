#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"

#include "../../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT_V1(ProcessorEntry, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(processor_type, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(enabled, meta::Reflect)
SE_END_REFLECT_V1(ProcessorEntry)
} // namespace se::editor
