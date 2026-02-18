#pragma once

#include "SimpleEngine/Asset/ImportConfig.h"
#include "SimpleEngine/Asset/Pipeline/PipelineNodeContainer.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::asset
{
/**
 * 소스 파일을 PipelineNode로 변환하는 인터페이스
 */
class SE_CORE_API IPipelineTranslator
{
public:
    virtual ~IPipelineTranslator() = default;

    /**
     * Translator가 지원하는 파일 확장자인지 확인합니다.
     * @param file_extension 파일 확장자 (ex: .obj)
     */
    [[nodiscard]] virtual bool CanTranslate(const String& file_extension) const = 0;

    /**
     * 파일을 읽고 Container에 Node를 채웁니다.
     * @param file_path 파일 경로
     * @param import_config 파일을 가져올 때 사용할 ImportConfig
     * @param out_container 변환된 노드들을 담을 컨테이너
     */
    virtual void Translate(
        const Path& file_path,
        const ImportConfig& import_config,
        PipelineNodeContainer& out_container
    ) = 0;
};
}  // namespace se::asset
