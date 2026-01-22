#pragma once
#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Asset/Pipeline/PipelineNodeContainer.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/Guid.h"


namespace se::asset
{
/**
 * Factory가 Asset을 생성할 때 필요한 의존성 정보를 담고 있는 Context
 */
struct PipelineImportContext
{
    const PipelineNodeContainer& container;
    const HashMap<Guid, std::shared_ptr<IAsset>>& created_assets;

    [[nodiscard]] std::shared_ptr<IAsset> GetCreatedAsset(const Guid& node_uid) const
    {
        return created_assets.Find(node_uid).ValueOr(nullptr);
    }
};

/**
 * Pipeline Node를 실제 IAsset으로 변환하는 Interface
 */
class IPipelineFactory
{
public:
    virtual ~IPipelineFactory() = default;

    /** 현재 Factory가 해당 노드를 처리할 수 있는지 확인합니다. */
    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const = 0;

    /**
     * 노드로부터 에셋을 생성합니다.
     * @param node 변환할 노드
     * @param context 다른 노드나 에셋에 접근하기 위한 Context
     * @return 생성된 에셋 (실패 시 nullptr)
     */
    virtual std::shared_ptr<IAsset> CreateAsset(
        const PipelineBaseNode* node,
        const PipelineImportContext& context
    ) = 0;

    /** 생성될 에셋의 타입 정보를 반환합니다. */
    [[nodiscard]] virtual refl::TypeId GetAssetType() const = 0;
};
}  // namespace se::asset
