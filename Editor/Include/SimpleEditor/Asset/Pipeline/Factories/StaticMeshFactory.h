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
    [[nodiscard]] virtual TypeId_v1 GetAssetType() const override;
    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const override;

    virtual std::shared_ptr<AssetBase> CreateAsset(PipelineBaseNode* node, const PipelineImportContext& context) override;
};
} // namespace se::editor
