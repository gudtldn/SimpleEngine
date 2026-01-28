#pragma once
#include <utility>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::ecs
{
/** Component를 담아두는 자료구조 입니다. */
template <typename ComponentType>
class SparseSet
{
private:
    /** EntityID -> DenseIdx */
    Array<Optional<usize>> sparse;

    /** Entity array */
    Array<Entity> dense;

    /** Component array */
    Array<ComponentType> components;

public:
    explicit SparseSet() = default;

    /** 엔티티에 컴포넌트를 추가하거나 갱신합니다. */
    void Add(Entity entity, ComponentType&& component)
    {
        if (entity.GetId() >= sparse.Len())
        {
            sparse.Resize(entity.GetId() + 1, std::nullopt);
        }

        // 이미 존재하면 덮어쓰기
        if (Contains(entity))
        {
            components[*sparse[entity.GetId()]] = std::move(component);
            return;
        }

        const usize dense_idx = dense.Len();
        sparse[entity.GetId()] = dense_idx;
        dense.Push(entity);
        components.Push(std::move(component));
    }

    /** 엔티티에 컴포넌트를 추가하거나 갱신합니다. */
    void Add(Entity entity, const ComponentType& component)
    {
        Add(entity, ComponentType{ component });
    }

    /** 엔티티가 가지고 있는 컴포넌트를 제거합니다. */
    void Remove(Entity entity)
    {
        if (!Contains(entity))
        {
            return;
        }

        const usize remove_idx = *sparse[entity.GetId()];
        const Entity last_entity = *dense.Back();

        // swap-remove
        dense[remove_idx] = last_entity;
        components[remove_idx] = std::move(*components.Back());

        sparse[last_entity.GetId()] = remove_idx;

        dense.Pop();
        components.Pop();
        sparse[entity.GetId()] = std::nullopt;
    }

    /** Set에 Entity가 있는지 확인합니다. */
    [[nodiscard]] bool Contains(Entity entity) const noexcept
    {
        if (entity.GetId() >= sparse.Len())
        {
            return false;
        }

        if (const Optional<usize> dense_idx_opt = sparse[entity.GetId()])
        {
            const auto dense_idx = *dense_idx_opt;
            return dense_idx < dense.Len() && dense[dense_idx] == entity;
        }
        return false;
    }

    /** Index로 Entity를 가져옵니다. */
    [[nodiscard]] Optional<Entity> GetEntityByIndex(usize index) const
    {
        // out of bounds 방지
        if (index < dense.Len())
        {
            return dense[index];
        }
        return std::nullopt;
    }

    /** Set에 등록된 Entity의 개수를 반환합니다. */
    [[nodiscard]] usize Length() const noexcept { return dense.Len(); }

    /** Set이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept { return dense.IsEmpty(); }

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

    template <typename Self>
    [[nodiscard]] Optional<traits::DeduceRetType<Self, ComponentType&>> TryGet(this Self&& self, Entity entity)
    {
        if (self.Contains(entity))
        {
            return self.components[*self.sparse[entity.GetId()]];
        }
        return std::nullopt;
    }

    /** Entity의 Component&를 반환합니다. */
    [[nodiscard]] ComponentType& Get(Entity entity)
    {
        Optional<ComponentType&> opt_value = TryGet(entity);
        SE_ASSERT(opt_value, "Entity does not exist");
        return *opt_value;
    }

    [[nodiscard]] const ComponentType& Get(Entity entity) const
    {
        Optional<ComponentType&> opt_value = TryGet(entity);
        SE_ASSERT(opt_value, "Entity does not exist");
        return *opt_value;
    }

    [[nodiscard]] const Array<Entity>& GetEntities() const { return dense; }
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
};

/** type erasure를 위한 인터페이스 */
class SE_CORE_API IStorage
{
public:
    virtual ~IStorage() = default;

    /** Storage의 크기를 반환합니다. */
    [[nodiscard]] virtual usize Length() const noexcept = 0;

    /** Storage가 비어있는지 확인합니다. */
    [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;

    /** 해당 엔티티가 컴포넌트를 가지고 있는지 확인합니다. */
    [[nodiscard]] virtual bool Contains(Entity entity) const noexcept = 0;

    /** Index로 Entity를 가져옵니다. */
    [[nodiscard]] virtual Optional<Entity> GetEntityByIndex(usize index) const = 0;

    /** 주어진 엔티티에 기본 생성자로 만들어진 컴포넌트를 추가합니다. */
    virtual void EmplaceDefault(Entity entity) = 0;

    /** 엔티티가 가지고 있는 컴포넌트를 제거합니다. */
    virtual void Remove(Entity entity) = 0;

    /** 컴포넌트 데이터의 Raw Pointer 반환 (리플렉션용) */
    [[nodiscard]] virtual void* GetRaw(Entity entity) = 0;
    [[nodiscard]] virtual const void* GetRaw(Entity entity) const = 0;
};

template <typename ComponentType>
class ComponentStorage : public IStorage
{
    SparseSet<ComponentType> storage;

public:
    ComponentStorage() = default;

    //~Begin IStorage
    [[nodiscard]] virtual usize Length() const noexcept override { return storage.Length(); }
    [[nodiscard]] virtual bool IsEmpty() const noexcept override { return storage.IsEmpty(); }
    [[nodiscard]] virtual bool Contains(Entity entity) const noexcept override { return storage.Contains(entity); }
    [[nodiscard]] virtual Optional<Entity> GetEntityByIndex(usize index) const override { return storage.GetEntityByIndex(index); }
    virtual void EmplaceDefault(Entity entity) override
    {
        static_assert(std::is_default_constructible_v<ComponentType>, "ComponentType must be default constructible");
        storage.Add(entity, ComponentType{});
    }
    virtual void Remove(Entity entity) override { storage.Remove(entity); }

    [[nodiscard]] virtual void* GetRaw(Entity entity) override
    {
        if (auto opt = storage.TryGet(entity))
        {
            return std::addressof(*opt);
        }
        return nullptr;
    }

    [[nodiscard]] virtual const void* GetRaw(Entity entity) const override
    {
        if (auto opt = storage.TryGet(entity))
        {
            return std::addressof(*opt);
        }
        return nullptr;
    }
    //~End IStorage

    template <typename Self>
    traits::DeduceRetType<Self, SparseSet<ComponentType>&> GetStorage(this Self&& self) { return self.storage; }
};
}  // namespace se::ecs
