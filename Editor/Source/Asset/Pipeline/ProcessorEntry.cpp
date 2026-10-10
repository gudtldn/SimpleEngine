#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::editor::ProcessorEntry, se::display::Hidden)
    SE_FIELD(processor_type)
    SE_FIELD(enabled)
SE_REFLECT_END()
