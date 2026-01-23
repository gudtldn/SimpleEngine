#pragma once
#include "SimpleEngine/Asset/Pipeline/Translators/IPipelineTranslator.h"


namespace se::asset
{
/**
 * Assimp 라이브러리를 사용하여 3D 모델 파일을 엔진 데이터 구조로 변환하는 클래스
 */
class SE_CORE_API AssimpTranslator : public IPipelineTranslator
{
public:
    [[nodiscard]] virtual bool CanTranslate(const std::filesystem::path& file_extension) const override;
    virtual void Translate(
        const std::filesystem::path& file_path,
        const ImportConfig& import_config,
        PipelineNodeContainer& out_container
    ) override;
};
}
