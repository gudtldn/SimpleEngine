export module SimpleEngine.Core:ECS.SparseSet;
import :ECS.ECSTypes;

import SimpleEngine.Types;
import std;


namespace se::core::ecs
{
/** Component를 담아두는 자료구조 입니다. */
template <typename ComponentType>
class SparseSet
{
    /** Index: EntityID, Value: DenseArrayIdx*/
    std::vector<std::optional<size_t>> sparse;

    /** EntityID array */
    std::vector<EntityId> dense;

    /** Component array */
    std::vector<ComponentType> components;

public:
    explicit SparseSet(size_t max_entities)
        : sparse(max_entities, std::nullopt)
    {
    }

    [[nodiscard]] bool Contains(EntityId entity_id) const noexcept
    {
        if (const std::optional<size_t> dense_idx_opt = sparse[entity_id])
        {
            const auto dense_idx = *dense_idx_opt;
            return dense_idx < dense.size() && dense[dense_idx] == entity_id;
        }
        return false;
    }

    void Add(EntityId entity_id, ComponentType&& component)
    {
        // 이미 존재하면 덮어쓰기
        if (Contains(entity_id))
        {
            components[sparse[entity_id]] = component;
            return;
        }

        const std::size_t dense_idx = dense.size();
        sparse[entity_id] = dense_idx;
        dense.push_back(entity_id);
        components.emplace_back(std::move(component));
    }

    void Remove(EntityId entity_id)
    {
        if (!Contains(entity_id))
        {
            return;
        }

        const size_t remove_entity = *sparse[entity_id];
        const uint32 last_entity = dense.back();

        // swap-remove
        dense[remove_entity] = last_entity;
        components[remove_entity] = std::move(components.back());
        sparse[last_entity] = remove_entity;

        dense.pop_back();
        components.pop_back();
        sparse[entity_id] = std::nullopt;
    }

    [[nodiscard]] std::optional<ComponentType&> Get(EntityId entity_id)
    {
        return Contains(entity_id) ? components[sparse[entity_id]] : std::nullopt;
    }

    [[nodiscard]] const std::vector<uint32>& GetEntities() const { return dense; }
    [[nodiscard]] const std::vector<ComponentType>& GetComponents() const { return components; }

public:
    [[nodiscard]] std::optional<ComponentType&> operator[](EntityId entity_id)
    {
        return Get(entity_id);
    }
};
}
