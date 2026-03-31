#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/ECS/ComponentStorage.h"
#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <memory>
#include <type_traits>
#include <utility>


namespace se
{
template <typename... Ts>
    requires QueryParameterPack<Ts...>
class Query;


/**
 * ECS 월드의 모든 요소(엔티티, 컴포넌트)를 관리하는 순수 데이터 클래스
 */
class SE_CORE_API World final
{
private:
    template <typename... Ts>
    friend class QueryData;

    friend class ComponentRegistry;

public:
    class EntityChain;

    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

public:
    /** Entity를 생성합니다. */
    template <typename... Components>
    EntityChain SpawnEntity(Components&&... comps)
    {
        const Entity new_entity = entity_manager.Create();
        alive_entities.Push(new_entity);

        EntityChain chain = { this, new_entity };
        (AddComponent(chain, std::forward<Components>(comps)), ...);
        return chain;
    }

    /** Entity와 연결된 모든 Component를 제거합니다. */
    void DestroyEntity(Entity entity);

    /** Entity가 현재 살아있는지 확인합니다. */
    [[nodiscard]] bool IsEntityAlive(Entity entity) const { return entity_manager.IsValid(entity); }

    /** 현재 살아있는 모든 Entity를 반환합니다. */
    [[nodiscard]] const Array<Entity>& GetAliveEntities() const { return alive_entities; }

public:
    /** Entity에 Component를 추가합니다. 이미 존재하면 덮어씌워집니다. */
    template <typename ComponentType>
    ComponentType& AddComponent(Entity entity, ComponentType&& init_component)
    {
        auto& storage = GetOrCreateStorage<ComponentType>();
        storage.Add(entity, std::forward<ComponentType>(init_component));
        return storage.Get(entity);
    }

    template <typename ComponentType, typename... Args>
    ComponentType& AddComponent(Entity entity, Args&&... args)
    {
        auto& storage = GetOrCreateStorage<ComponentType>();
        storage.Add(entity, ComponentType{ std::forward<Args>(args)... });
        return storage.Get(entity);
    }

    /** Entity에서 Component를 제거합니다. */
    template <typename ComponentType>
    void RemoveComponent(Entity entity)
    {
        if (Optional storage_opt = GetStorage<ComponentType>())
        {
            storage_opt->Remove(entity);
        }
    }

    /** Entity에서 ComponentType에 맞는 Component를 참조로 가져옵니다. */
    template <typename ComponentType, typename Self>
    traits::CopyConst<Self, ComponentType&> GetComponent(this Self&& self, Entity entity)
    {
        return self.template GetStorage<ComponentType>()->Get(entity);
    }

    /** Entity에서 ComponentType에 맞는 Component를 Optional로 가져옵니다. */
    template <typename ComponentType, typename Self>
    Optional<traits::CopyConst<Self, ComponentType&>> TryGetComponent(this Self&& self, Entity entity)
    {
        if (const auto opt_storage = self.template GetStorage<ComponentType>())
        {
            return opt_storage->Find(entity);
        }
        return NullOpt;
    }

    /** Entity가 특정 Component를 가지고 있는지 확인합니다. */
    template <typename ComponentType>
    [[nodiscard]] bool HasComponent(Entity entity) const
    {
        if (Optional opt_storage = GetStorage<ComponentType>())
        {
            return opt_storage->Contains(entity);
        }
        return false;
    }

public:
    /**
     * Entity를 쿼리하는 Query 객체를 생성합니다.
     * @tparam Ts Component 목록 및 필터(With<...>, Without<...>)
     */
    template <typename... Ts>
        requires requires { Query<Ts...>{ std::declval<World*>() }; }
    [[nodiscard]] Query<Ts...> QueryEntities() // TODO: QueryEntities를 const로 만들기
    {
        return Query<Ts...>{ this };
    }

    /**
     * 주어진 TypeId에 해당하는 IStorage 포인터를 반환합니다.
     * @param type_id 검색할 타입의 TypeId
     * @return IStorage 포인터, 해당 타입이 없을 경우 nullptr 반환
     */
    IStorage* GetStorage(const TypeId& type_id);

private:
    template <typename ComponentType>
    ComponentStorage<std::decay_t<ComponentType>>& GetOrCreateStorageImpl()
    {
        using RawType = std::decay_t<ComponentType>;

        const auto type_id = TypeId::Get<RawType>();
        auto& storage_ptr = component_storages
            .Entry(type_id)
            .OrInsertWith([]{ return std::make_unique<ComponentStorage<RawType>>(); });

        return *static_cast<ComponentStorage<RawType>*>(storage_ptr.get());
    }

    template <typename ComponentType>
    SparseSet<std::decay_t<ComponentType>>& GetOrCreateStorage()
    {
        return GetOrCreateStorageImpl<ComponentType>().GetStorage();
    }

    template <typename ComponentType, typename Self>
    Optional<traits::CopyConst<Self, SparseSet<std::decay_t<ComponentType>>&>> GetStorage(this Self&& self)
    {
        using RawType = std::decay_t<ComponentType>;
        if (traits::CopyConst<Self, IStorage*> storage = self.template GetIStorage<RawType>())
        {
            return static_cast<traits::CopyConst<Self, ComponentStorage<RawType>*>>(storage)->GetStorage();
        }
        return NullOpt;
    }

    /** 타입에 맞는 IStorage 포인터를 반환합니다. 쿼리 시스템 내부에서 사용됩니다. */
    template <typename ComponentType, typename Self>
    traits::CopyConst<Self, IStorage*> GetIStorage(this Self&& self)
    {
        using RawType = std::decay_t<ComponentType>;

        const auto type_id = TypeId::Get<RawType>();
        if (Optional storage_opt = self.component_storages.Find(type_id))
        {
            return storage_opt->get();
        }
        return nullptr;
    }

public:
    class EntityChain
    {
    public:
        EntityChain(World* in_world, Entity new_entity)
            : world(in_world)
            , entity(new_entity)
        {
        }

        /** Entity에 Component를 추가합니다. 이미 존재하면 덮어씌워집니다. */
        template <typename ComponentType>
        EntityChain& AddComponent(ComponentType&& init_component)
        {
            world->AddComponent(entity, std::forward<ComponentType>(init_component));
            return *this;
        }

        template <typename ComponentType, typename... Args>
        EntityChain& AddComponent(Args&&... args)
        {
            world->AddComponent<ComponentType>(entity, std::forward<Args>(args)...);
            return *this;
        }

        operator Entity() const { return entity; }

    private:
        World* world;
        Entity entity;
    };

private:
    EntityManager entity_manager;
    Array<Entity> alive_entities;

    HashMap<TypeId, std::unique_ptr<IStorage>> component_storages;
};
} // namespace se
