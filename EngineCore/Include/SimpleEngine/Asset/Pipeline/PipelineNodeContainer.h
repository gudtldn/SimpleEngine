#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineBaseNode.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/Guid.h"


namespace se::asset
{
/**
 * @todo docs
 */
class PipelineNodeContainer
{
public:
    template <typename T, typename... Args>
        requires std::derived_from<T, PipelineBaseNode>
    T* CreateNode(Args&&... args)
    {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        if (!node->GetUid().IsValid())
        {
            node->SetUid(Guid::NewGuid());
        }

        T* ptr = node.get();
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

    [[nodiscard]] const auto& GetAllNodes() const { return nodes; }

private:
    HashMap<Guid, std::unique_ptr<PipelineBaseNode>> nodes;
};
}  // namespace se::asset
