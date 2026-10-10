#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::editor::PipelineBaseNode)
    SE_FIELD(self_uid)
    SE_FIELD(parent_uid)
    SE_FIELD(display_name)
    SE_FIELD(attributes)
SE_REFLECT_END()
