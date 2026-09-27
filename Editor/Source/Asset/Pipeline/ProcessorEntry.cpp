#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "../../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se::editor
{
SE_BEGIN_REFLECT_V1(ProcessorEntry, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(processor_type, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(enabled, meta::Reflect)
SE_END_REFLECT_V1(ProcessorEntry)
} // namespace se::editor


// processor_type의 TypeId_v1은 AssetMetadata.h에서 Opaque로 등록됨
SE_REFLECT_BEGIN(se::editor::ProcessorEntry)
    SE_FIELD(processor_type)
    SE_FIELD(enabled)
SE_REFLECT_END()
