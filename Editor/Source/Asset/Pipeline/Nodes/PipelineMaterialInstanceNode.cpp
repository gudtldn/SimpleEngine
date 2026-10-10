#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineMaterialInstanceNode.h"
#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se::editor
{
void PipelineMaterialInstanceNode::GetFactoryDependencies(Array<Guid>& out_dependencies) const
{
    PipelineBaseNode::GetFactoryDependencies(out_dependencies);
    out_dependencies.PushRange(texture_node_refs | std::views::values);
}
} // namespace se::editor


SE_REFLECT_BEGIN(se::editor::PipelineMaterialInstanceNode)
    SE_BASE(se::editor::PipelineBaseNode)
    SE_FIELD(texture_node_refs)
    SE_FIELD(param_overrides)
    SE_FIELD(blend_mode_override)
    SE_FIELD(two_sided_override)
SE_REFLECT_END()
