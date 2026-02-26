#pragma once

#include "SimpleEditor/Asset/Pipeline/Translators/IPipelineTranslator.h"


namespace se::editor
{
/**
 * Assimp 라이브러리를 사용하여 3D 모델 파일을 엔진 데이터 구조로 변환하는 클래스
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) AssimpTranslator : public IPipelineTranslator
{
    SE_CLASS(AssimpTranslator)

public:
    [[nodiscard]] virtual ArrayView<const StringView> GetSupportedExtensions() const override;

    virtual void Translate(
        const Path& file_path,
        const ImportProfile& import_profile,
        PipelineNodeContainer& out_container
    ) override;
};
} // namespace se::editor
