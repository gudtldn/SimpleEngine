#include "SimpleEditor/Asset/MetaFileContent.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::editor::MetaFileContent, se::display::Hidden)
    SE_FIELD(metadata)
    SE_FIELD(import_settings)
    SE_FIELD(processor_stack)
SE_REFLECT_END()
