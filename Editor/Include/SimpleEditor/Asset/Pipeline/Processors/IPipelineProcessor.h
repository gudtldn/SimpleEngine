#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"


namespace se::editor
{
/**
 * Translator가 생성한 Node Graph를 가공하는 Interface
 */
class SE_EDITOR_API IPipelineProcessor
{
public:
    virtual ~IPipelineProcessor() = default;

    /**
     * 노드 컨테이너를 받아 내부 데이터를 수정합니다.
     * @param in_out_container 수정될 노드 컨테이너
     */
    virtual void Process(PipelineNodeContainer& in_out_container) = 0;
};
} // namespace se::editor
