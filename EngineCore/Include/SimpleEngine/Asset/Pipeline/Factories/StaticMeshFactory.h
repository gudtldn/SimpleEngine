#pragma once
#include "SimpleEngine/Asset/Pipeline/Factories/IPipelineFactory.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API StaticMeshFactory : public IPipelineFactory
{
public:
    [[nodiscard]] virtual refl::TypeId GetAssetType() const override;

    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const override;
    virtual std::shared_ptr<IAsset> CreateAsset(PipelineBaseNode* node, const PipelineImportContext& context) override;
};
}
