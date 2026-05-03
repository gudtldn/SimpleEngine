#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <memory>


namespace se::editor
{
/**
 * 파이프라인 시스템에서 사용되는 노드들의 생성 및 생명주기를 관리하는 컨테이너 클래스
 */
class SE_EDITOR_API PipelineNodeContainer
{
public:
    using NodeMap = HashMap<Guid, std::unique_ptr<PipelineBaseNode>>;

    PipelineNodeContainer() = default;
    ~PipelineNodeContainer() = default;

    // 복사만 금지
    PipelineNodeContainer(const PipelineNodeContainer&) = delete;
    PipelineNodeContainer& operator=(const PipelineNodeContainer&) = delete;
    PipelineNodeContainer(PipelineNodeContainer&&) = default;
    PipelineNodeContainer& operator=(PipelineNodeContainer&&) = default;

public:
    template <typename NodeType, typename... Args>
        requires std::derived_from<NodeType, PipelineBaseNode>
    NodeType& CreateNode(Args&&... args)
    {
        auto node = std::make_unique<NodeType>(std::forward<Args>(args)...);
        if (!node->GetUid().IsValid())
        {
            node->SetUid(Guid::NewGuid());
        }

        NodeType* node_ptr = node.get();
        nodes.Insert(node_ptr->GetUid(), std::move(node));
        return *node_ptr;
    }

    // v2 Translator 전용: GUID를 사전 지정하여 node->GetUid() == AssetId.guid 불변식을 보장합니다.
    template <typename NodeType, typename... Args>
        requires std::derived_from<NodeType, PipelineBaseNode>
    NodeType& CreateNode(const Guid& pre_assigned_uid, Args&&... args)
    {
        auto node = std::make_unique<NodeType>(std::forward<Args>(args)...);
        node->SetUid(pre_assigned_uid.IsValid() ? pre_assigned_uid : Guid::NewGuid());

        NodeType* node_ptr = node.get();
        nodes.Insert(node_ptr->GetUid(), std::move(node));
        return *node_ptr;
    }

    [[nodiscard]] Optional<PipelineBaseNode&> GetNode(const Guid& uid) const
    {
        return nodes.Find(uid).AndThen([](auto& ptr) -> Optional<PipelineBaseNode&>
        {
            return *ptr;
        });
    }

    [[nodiscard]] PipelineBaseNode& GetNodeChecked(const Guid& uid) const
    {
        SE_ASSERT(nodes.Contains(uid), "Node with UID {} does not exist!", uid);
        return *nodes.FindChecked(uid);
    }

    template <typename NodeType>
        requires std::derived_from<NodeType, PipelineBaseNode>
    [[nodiscard]] Optional<NodeType&> GetNode(const Guid& uid) const
    {
        return GetNode(uid).AndThen([](PipelineBaseNode& node) -> Optional<NodeType&>
        {
            if (NodeType* casted = Cast<NodeType>(&node))
            {
                return *casted;
            }
            return {};
        });
    }

    template <typename NodeType>
        requires std::derived_from<NodeType, PipelineBaseNode>
    [[nodiscard]] NodeType& GetNodeChecked(const Guid& uid) const
    {
        auto& node = GetNodeChecked(uid);
        return *CastChecked<NodeType>(&node);
    }

    [[nodiscard]] bool Contains(const Guid& uid) const
    {
        return nodes.Contains(uid);
    }

    [[nodiscard]] const NodeMap& GetAllNodes() const
    {
        return nodes;
    }

private:
    NodeMap nodes;
};
} // namespace se::editor
