#pragma once
#include "SimpleEngine/Asset/Pipeline/PipelineNodeContainer.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API IPipelineTranslator
{
public:
    virtual ~IPipelineTranslator() = default;

    /** Translator가 지원하는 파일 확장자인지를 반환합니다. */
    [[nodiscard]] virtual bool CanTranslate(const String& file_extension) const = 0;

    /** 파일을 읽고 Container에 Node를 채웁니다. */
    virtual void Translate(const String& file_path, PipelineNodeContainer& out_container) = 0;
};
}  // namespace se::asset
