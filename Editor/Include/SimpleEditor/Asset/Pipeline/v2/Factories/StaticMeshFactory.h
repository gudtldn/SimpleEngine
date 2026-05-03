#pragma once

#include "SimpleEditor/Asset/Pipeline/v2/Factories/IPipelineFactory.h"


namespace se::editor::v2
{
/**
 * @todo docs
 */
class SE_EDITOR_API StaticMeshFactory : public IPipelineFactory
{
public:
    [[nodiscard]] virtual TypeId GetAssetType() const override;
    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const override;

    virtual std::shared_ptr<AssetBase> CreateAsset(
        PipelineBaseNode* node,
        const PipelineImportContext& context
    ) override;
};
} // namespace se::editor::v2
