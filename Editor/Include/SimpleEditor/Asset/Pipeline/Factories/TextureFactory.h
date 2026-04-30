#pragma once

#include "SimpleEditor/Asset/Pipeline/Factories/IPipelineFactory.h"


namespace se::editor
{
/**
 * PipelineTextureNode를 Texture2D 에셋으로 변환하는 Factory
 */
class SE_EDITOR_API TextureFactory : public IPipelineFactory
{
public:
    [[nodiscard]] virtual TypeId GetAssetType() const override;
    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const override;

    virtual std::shared_ptr<asset::AssetBase> CreateAsset(PipelineBaseNode* node, const PipelineImportContext& context) override;
};
} // namespace se::editor
