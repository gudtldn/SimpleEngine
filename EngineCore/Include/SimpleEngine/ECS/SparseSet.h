#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <utility>


namespace se
{
/** Entity와 Component를 쌍으로 저장하는 Sparse Set 자료구조 */
template <typename ComponentType>
class SparseSet
{
public:
    explicit SparseSet() = default;

    /** 엔티티에 컴포넌트를 추가하거나 갱신합니다. */
    template <typename T>
        requires std::constructible_from<ComponentType, T&&>
    void Add(Entity entity, T&& component)
    {
        if (entity.GetId() >= sparse.Len())
        {
            sparse.Resize(entity.GetId() + 1, INVALID_INDEX);
        }

        // 이미 존재하면 덮어쓰기
        if (Contains(entity))
        {
            components[sparse[entity.GetId()]] = std::forward<T>(component);
            return;
        }

        const usize dense_idx = dense.Len();
        sparse[entity.GetId()] = dense_idx;
        dense.Push(entity);
        components.Push(std::forward<T>(component));
    }

    /** 엔티티가 가지고 있는 컴포넌트를 제거합니다. */
    void Remove(Entity entity)
    {
        if (!Contains(entity))
        {
            return;
        }

        const usize remove_idx = sparse[entity.GetId()];
        const Entity last_entity = *dense.Back();

        // swap-remove
        dense[remove_idx] = last_entity;
        components[remove_idx] = std::move(*components.Back());

        sparse[last_entity.GetId()] = remove_idx;

        dense.Pop();
        components.Pop();
        sparse[entity.GetId()] = INVALID_INDEX;
    }

    /** Set에 Entity가 있는지 확인합니다. */
    [[nodiscard]] bool Contains(Entity entity) const noexcept
    {
        if (entity.GetId() >= sparse.Len())
        {
            return false;
        }

        const auto dense_idx = sparse[entity.GetId()];
        return dense_idx < dense.Len() && dense[dense_idx] == entity;
    }

    /** Dense 배열의 인덱스로 Entity를 가져옵니다. */
    [[nodiscard]] Optional<Entity> GetEntityByIndex(usize index) const
    {
        // out of bounds 방지
        if (index < dense.Len())
        {
            return dense[index];
        }
        return NullOpt;
    }

    /** Set에 등록된 Entity의 개수를 반환합니다. */
    [[nodiscard]] usize Len() const noexcept { return dense.Len(); }

    /** Set이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept { return dense.IsEmpty(); }

    /** Entity의 Component를 가져오려고 시도합니다. */
    template <typename Self>
    [[nodiscard]] Optional<traits::CopyConst<Self, ComponentType&>> Find(this Self&& self, Entity entity)
    {
        if (self.Contains(entity))
        {
            return self.components[self.sparse[entity.GetId()]];
        }
        return NullOpt;
    }

    /**
     * Entity의 Component 참조를 반환합니다.
     * @note 존재하지 않는 entity의 경우 Assert
     */
    template <typename Self>
    [[nodiscard]] traits::CopyConst<Self, ComponentType&> Get(this Self&& self, Entity entity)
    {
        auto opt_value = self.Find(entity);
        SE_ASSERT(opt_value, "Entity does not exist");
        return *opt_value;
    }

    /** Dense 엔티티 배열을 반환합니다. */
    [[nodiscard]] const Array<Entity>& GetEntities() const { return dense; }

    /** Dense 컴포넌트 배열을 반환합니다. */
    [[nodiscard]] const Array<ComponentType>& GetComponents() const { return components; }

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
        using difference_type = isize;
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

    using IteratorType = SparseSetIteratorTemplate<Array<Entity>::IteratorType, typename Array<ComponentType>::IteratorType>;
    using ConstIteratorType = SparseSetIteratorTemplate<Array<Entity>::ConstIteratorType, typename Array<ComponentType>::ConstIteratorType>;

    [[nodiscard]] IteratorType begin() { return IteratorType(dense.begin(), components.begin()); }
    [[nodiscard]] IteratorType end() { return IteratorType(dense.end(), components.end()); }
    [[nodiscard]] ConstIteratorType begin() const { return ConstIteratorType(dense.begin(), components.begin()); }
    [[nodiscard]] ConstIteratorType end() const { return ConstIteratorType(dense.end(), components.end()); }

private:
    static constexpr usize INVALID_INDEX = std::numeric_limits<usize>::max();

    /** EntityID -> DenseIdx */
    Array<usize> sparse;

    /** Entity array */
    Array<Entity> dense;

    /** Component array */
    Array<ComponentType> components;
};
} // namespace se
