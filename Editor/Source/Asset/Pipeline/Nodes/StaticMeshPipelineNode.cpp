#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_BEGIN(se::editor::PipelineMeshSection)
    SE_FIELD(index_offset)
    SE_FIELD(index_count)
    SE_FIELD(vertex_offset)
    SE_FIELD(vertex_count)
    SE_FIELD(material_index)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::StaticMeshPipelineNode)
    SE_BASE(se::editor::PipelineBaseNode)
    SE_FIELD(vertices)
    SE_FIELD(indices)
    SE_FIELD(sections)
    SE_FIELD(local_transform)
    SE_FIELD(material_node_uids)
SE_REFLECT_END()
