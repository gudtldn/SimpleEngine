#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"
#include "SimpleEditor/Asset/Pipeline/Processors/IPipelineProcessor.h"

#include "SimpleEngine/Core/Container/Array.h"


namespace se::editor
{
/**
 * 여러 Processor들을 순서대로 실행하는 Stack Container
 */
class SE_EDITOR_API PipelineProcessorStack
{
public:
    PipelineProcessorStack() = default;
    ~PipelineProcessorStack() = default;

    // 복사만 금지
    PipelineProcessorStack(const PipelineProcessorStack&) = delete;
    PipelineProcessorStack& operator=(const PipelineProcessorStack&) = delete;
    PipelineProcessorStack(PipelineProcessorStack&&) = default;
    PipelineProcessorStack& operator=(PipelineProcessorStack&&) = default;

public:
    /** 프로세서를 Stack 끝에 추가합니다. */
    template <typename T, typename... Args>
    void AddProcessor(Args&&... args)
    {
        processors.Push(std::make_unique<T>(std::forward<Args>(args)...));
    }

    /** 이미 생성된 프로세서 인스턴스를 Stack 끝에 추가합니다. */
    void AddProcessor(std::unique_ptr<IPipelineProcessor> processor)
    {
        processors.Push(std::move(processor));
    }

    /** Stack에 있는 모든 프로세서를 순차적으로 실행하여 노드를 가공합니다. */
    void ExecuteStack(PipelineNodeContainer& container) const
    {
        for (const auto& processor : processors)
        {
            // 로그: processor->GetDisplayName() 실행 중...
            processor->Process(container);
        }
    }

    /** Stack에 있는 모든 프로세서를 제거합니다. */
    void Clear()
    {
        processors.Clear();
    }

private:
    Array<std::unique_ptr<IPipelineProcessor>> processors;
};
} // namespace se::editor
