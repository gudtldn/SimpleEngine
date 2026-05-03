#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"
#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"

#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"

#include <memory>


namespace se::editor
{
/**
 * Factory가 에셋을 생성할 때 필요한 컨텍스트
 */
struct SE_EDITOR_API PipelineImportContext
{
    const PipelineNodeContainer& container;
    const AssetRegistry& registry;
};

/**
 * PipelineNode를 실제 AssetBase로 생성하는 Interface
 */
class SE_EDITOR_API IPipelineFactory
{
public:
    virtual ~IPipelineFactory() = default;

    /** 생성될 에셋의 타입 정보를 반환합니다. */
    [[nodiscard]] virtual TypeId GetAssetType() const = 0;

    /** 현재 Factory가 해당 노드를 처리할 수 있는지 확인합니다. */
    [[nodiscard]] virtual bool CanCreateAsset(const PipelineBaseNode* node) const = 0;

    /**
     * 노드로부터 에셋을 생성합니다.
     * @param node 변환할 노드
     * @param context 다른 노드나 에셋에 접근하기 위한 Context
     * @return 생성된 에셋 (실패 시 nullptr)
     */
    virtual std::shared_ptr<AssetBase> CreateAsset(
        PipelineBaseNode* node,
        const PipelineImportContext& context
    ) = 0;
};
} // namespace se::editor
