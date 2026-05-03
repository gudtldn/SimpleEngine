#pragma once

#include "SimpleEditor/Asset/Pipeline/v2/Translators/IPipelineTranslator.h"


namespace se::editor::v2
{
/**
 * Assimp를 사용하여 3D 모델 파일을 PipelineNode로 변환하는 Translator
 */
class SE_EDITOR_API AssimpTranslator : public IPipelineTranslator
{
public:
    [[nodiscard]] virtual ArrayView<const StringView> GetSupportedExtensions() const override;

    virtual void Translate(
        const Path& file_path,
        const ImportProfile& import_profile,
        ImportContext& io_ctx,
        PipelineNodeContainer& out_container
    ) override;
};
} // namespace se::editor::v2
