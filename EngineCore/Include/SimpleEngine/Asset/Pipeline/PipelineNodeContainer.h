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
class SE_CORE_API PipelineNodeContainer
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

        NodeType* ptr = node.get();
        nodes.Insert(node->GetUid(), std::move(node));
        return *ptr;
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
            return static_cast<NodeType&>(node);
        });
    }

    template <typename NodeType>
        requires std::derived_from<NodeType, PipelineBaseNode>
    [[nodiscard]] NodeType& GetNodeChecked(const Guid& uid) const
    {
        const auto& node = GetNodeChecked(uid);
        SE_ASSERT(
            node.GetTypeId() == refl::TypeId::Get<NodeType>(),
            "Node Type Mismatch! Expected: {}, Actual: {}",
            refl::TypeId::Get<NodeType>().GetName(), node.GetTypeId().GetName()
        );
        return static_cast<NodeType&>(node);
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
}  // namespace se::asset
