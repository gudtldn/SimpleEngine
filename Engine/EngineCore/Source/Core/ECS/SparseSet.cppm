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
private:
    /** EntityID -> DenseIdx */
    std::vector<Optional<size_t>> sparse;

    /** Entity array */
    std::vector<Entity> dense;

    /** Component array */
    std::vector<ComponentType> components;

public:
    explicit SparseSet(size_t max_entities)
        : sparse(max_entities, std::nullopt)
    {
    }

    /** 엔티티에 컴포넌트를 추가하거나 갱신합니다. */
    void Add(Entity entity, ComponentType&& component)
    {
        assert(entity.GetId() < sparse.size() && "Entity ID is out of range");

        // 이미 존재하면 덮어쓰기
        if (Contains(entity))
        {
            components[*sparse[entity.GetId()]] = std::move(component);
            return;
        }

        const std::size_t dense_idx = dense.size();
        sparse[entity.GetId()] = dense_idx;
        dense.push_back(entity);
        components.emplace_back(std::move(component));
    }

    /** 엔티티가 가지고 있는 컴포넌트를 제거합니다. */
    void Remove(Entity entity)
    {
        if (!Contains(entity))
        {
            return;
        }

        const size_t remove_idx = *sparse[entity.GetId()];
        const Entity last_entity = dense.back();

        // swap-remove
        dense[remove_idx] = last_entity;
        components[remove_idx] = std::move(components.back());

        sparse[last_entity.GetId()] = remove_idx;

        dense.pop_back();
        components.pop_back();
        sparse[entity.GetId()] = std::nullopt;
    }

    /** Set에 Entity가 있는지 확인합니다. */
    [[nodiscard]] bool Contains(Entity entity) const noexcept
    {
        if (entity.GetId() >= sparse.size())
        {
            return false;
        }

        if (const Optional<size_t> dense_idx_opt = sparse[entity.GetId()])
        {
            const auto dense_idx = *dense_idx_opt;
            return dense_idx < dense.size() && dense[dense_idx] == entity;
        }
        return false;
    }

    /** Index로 Entity를 가져옵니다. */
    [[nodiscard]] Optional<Entity> GetEntityByIndex(size_t index) const
    {
        // out of bounds 방지
        if (index >= sparse.size())
        {
            return std::nullopt;
        }

        if (const Optional<size_t> dense_idx_opt = sparse[index])
        {
            const auto dense_idx = *dense_idx_opt;
            return dense[dense_idx];
        }
        return std::nullopt;
    }

    /** Set에 등록된 Entity의 개수를 반환합니다. */
    [[nodiscard]] size_t Length() const noexcept { return dense.size(); }

    /** Set이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept { return dense.empty(); }

    /** Entity의 Component를 가져오려고 시도합니다. */
    [[nodiscard]] Optional<ComponentType&> TryGet(Entity entity)
    {
        if (Contains(entity))
        {
            return components[*sparse[entity.GetId()]];
        }
        return std::nullopt;
    }

    [[nodiscard]] Optional<const ComponentType&> TryGet(Entity entity) const
    {
        if (Contains(entity))
        {
            return components[*sparse[entity.GetId()]];
        }
        return std::nullopt;
    }

    /** Entity의 Component&를 반환합니다. */
    [[nodiscard]] ComponentType& Get(Entity entity)
    {
        Optional<ComponentType&> opt_value = TryGet(entity);
        assert(opt_value && "Entity does not exist");
        return *opt_value;
    }

    [[nodiscard]] const ComponentType& Get(Entity entity) const
    {
        Optional<ComponentType&> opt_value = TryGet(entity);
        assert(opt_value && "Entity does not exist");
        return *opt_value;
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
