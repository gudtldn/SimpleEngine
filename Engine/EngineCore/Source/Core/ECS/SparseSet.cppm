export module SimpleEngine.Core:ECS.SparseSet;
import :ECS.Entity;

import SimpleEngine.Types;
import std;

import <cassert>;


namespace se::core::ecs
{
/** Component를 담아두는 자료구조 입니다. */
template <typename ComponentType>
class SparseSet
{
    /** EntityID -> DenseIdx */
    std::vector<std::optional<size_t>> sparse;

    /** Entity array */
    std::vector<Entity> dense;

    /** Component array */
    std::vector<ComponentType> components;

public:
    explicit SparseSet(size_t max_entities)
        : sparse(max_entities, std::nullopt)
    {
    }

    [[nodiscard]] bool Contains(Entity entity) const noexcept
    {
        if (entity.id >= sparse.size())
        {
            return false;
        }

        if (const std::optional<size_t> dense_idx_opt = sparse[entity.id])
        {
            const auto dense_idx = *dense_idx_opt;
            return dense_idx < dense.size() && dense[dense_idx] == entity;
        }
        return false;
    }

    void Add(Entity entity, ComponentType&& component)
    {
        assert(entity.id < sparse.size() && "Entity ID is out of range");

        // 이미 존재하면 덮어쓰기
        if (Contains(entity))
        {
            components[*sparse[entity.id]] = std::move(component);
            return;
        }

        const std::size_t dense_idx = dense.size();
        sparse[entity.id] = dense_idx;
        dense.push_back(entity);
        components.emplace_back(std::move(component));
    }

    void Remove(Entity entity)
    {
        if (!Contains(entity))
        {
            return;
        }

        const size_t remove_idx = *sparse[entity.id];
        const Entity last_entity = dense.back();

        // swap-remove
        dense[remove_idx] = last_entity;
        components[remove_idx] = std::move(components.back());

        sparse[last_entity.id] = remove_idx;

        dense.pop_back();
        components.pop_back();
        sparse[entity.id] = std::nullopt;
    }

    [[nodiscard]] ComponentType* TryGet(Entity entity)
    {
        return Contains(entity) ? &components[*sparse[entity.id]] : nullptr;
    }

    [[nodiscard]] ComponentType& Get(Entity entity)
    {
        ComponentType* ptr = TryGet(entity);
        assert(ptr && "Entity does not exist");
        return *ptr;
    }

    [[nodiscard]] const std::vector<Entity>& GetEntities() const { return dense; }
    [[nodiscard]] const std::vector<ComponentType>& GetComponents() const { return components; }

    [[nodiscard]] ComponentType& operator[](Entity entity_id) { return Get(entity_id); }
    [[nodiscard]] const ComponentType& operator[](Entity entity_id) const { return Get(entity_id); }

public:
    template <typename EntityIt, typename CompIt>
    class SparseSetIteratorTemplate
    {
    public:
        template <typename C>
        struct EntityComponentPair
        {
            Entity entity;
            C& component;
        };

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = EntityComponentPair<typename std::iterator_traits<CompIt>::value_type>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

    public:
        SparseSetIteratorTemplate(EntityIt in_entity_iter, CompIt in_comp_iter)
            : entity_iter(std::move(in_entity_iter))
            , comp_iter(std::move(in_comp_iter))
        {
        }

        value_type operator*() const
        {
            return { *entity_iter, *comp_iter };
        }

        SparseSetIteratorTemplate& operator++()
        {
            ++entity_iter;
            ++comp_iter;
            return *this;
        }

        SparseSetIteratorTemplate operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const SparseSetIteratorTemplate& other) const
        {
            return entity_iter == other.entity_iter && comp_iter == other.comp_iter;
        }

        bool operator!=(const SparseSetIteratorTemplate& other) const
        {
            return !(*this == other);
        }

    private:
        EntityIt entity_iter;
        CompIt comp_iter;
    };

    using Iterator = SparseSetIteratorTemplate<std::vector<Entity>::iterator, typename std::vector<ComponentType>::iterator>;
    using ConstIterator = SparseSetIteratorTemplate<std::vector<Entity>::const_iterator, typename std::vector<ComponentType>::const_iterator>;

    [[nodiscard]] Iterator begin() { return Iterator(dense.begin(), components.begin()); }
    [[nodiscard]] Iterator end() { return Iterator(dense.end(), components.end()); }
    [[nodiscard]] ConstIterator begin() const { return ConstIterator(dense.cbegin(), components.cbegin()); }
    [[nodiscard]] ConstIterator end() const { return ConstIterator(dense.cend(), components.cend()); }
};
}
