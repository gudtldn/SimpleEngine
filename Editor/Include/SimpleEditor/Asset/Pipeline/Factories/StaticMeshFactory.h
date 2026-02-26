#pragma once
#include "SimpleEditor/Asset/Pipeline/Factories/IPipelineFactory.h"


namespace se::editor
{
/**
 * @todo docs
 */
class SE_EDITOR_API StaticMeshFactory : public IPipelineFactory
{
public:
    [[nodiscard]] virtual TypeId GetAssetType() const override;

    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const override;
    virtual std::shared_ptr<asset::AssetBase> CreateAsset(PipelineBaseNode* node, const PipelineImportContext& context) override;
};
} // namespace se::editor
