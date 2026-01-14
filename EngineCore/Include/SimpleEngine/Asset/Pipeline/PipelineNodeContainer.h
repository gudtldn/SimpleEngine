#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineBaseNode.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::asset
{
/**
 * @todo docs
 */
class PipelineNodeContainer
{
public:
    template <typename NodeType, typename... Args>
        requires std::derived_from<NodeType, PipelineBaseNode>
    NodeType* CreateNode(Args&&... args)
    {
        auto node = std::make_unique<NodeType>(std::forward<Args>(args)...);
        if (!node->GetUid().IsValid())
        {
            node->SetUid(Guid::NewGuid());
        }

        NodeType* ptr = node.get();
        nodes.Insert(node->GetUid(), std::move(node));
        return ptr;
    }

    [[nodiscard]] PipelineBaseNode* GetNode(const Guid& uid) const
    {
        if (const Optional ptr = nodes.Find(uid))
        {
            return ptr->get();
        }
        return nullptr;
    }

    template <typename NodeType>
        requires std::derived_from<NodeType, PipelineBaseNode>
    [[nodiscard]] NodeType* GetNode(const Guid& uid) const
    {
        PipelineBaseNode* node = GetNode(uid);
        if (node && node->GetTypeId() == refl::TypeId::Get<NodeType>())
        {
            return static_cast<NodeType*>(node);
        }
        return nullptr;
    }

    template <typename NodeType>
        requires std::derived_from<NodeType, PipelineBaseNode>
    [[nodiscard]] NodeType* GetNodeChecked(const Guid& uid) const
    {
        PipelineBaseNode* node = GetNode(uid);
        SE_ASSERT(node, "Node with UID {} does not exist!", uid.ToString());
        SE_ASSERT(
            node->GetTypeId() == refl::TypeId::Get<NodeType>(),
            "Node Type Mismatch! Expected: {}, Actual: {}",
            refl::TypeId::Get<NodeType>().GetName(), node->GetTypeId().GetName()
        );
        return static_cast<NodeType*>(node);
    }

    [[nodiscard]] const auto& GetAllNodes() const { return nodes; }

private:
    HashMap<Guid, std::unique_ptr<PipelineBaseNode>> nodes;
};
}  // namespace se::asset
