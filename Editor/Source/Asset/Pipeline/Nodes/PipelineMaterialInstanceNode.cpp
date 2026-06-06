#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineMaterialInstanceNode.h"


namespace se::editor
{
SE_BEGIN_REFLECT(PipelineMaterialInstanceNode, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(PipelineMaterialInstanceNode)

void PipelineMaterialInstanceNode::GetFactoryDependencies(Array<Guid>& out_dependencies) const
{
    PipelineBaseNode::GetFactoryDependencies(out_dependencies);
    out_dependencies.PushRange(texture_node_refs | std::views::values);
}
} // namespace se::editor
