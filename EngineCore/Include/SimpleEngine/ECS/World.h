#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/ECS/ComponentStorage.h"
#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/ECS/ResourceStorage.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <memory>
#include <type_traits>
#include <utility>


namespace se
{
// forward declarations
class CommandBuffer;

template <typename... Ts>
class Query;


/**
 * ECS 월드의 모든 요소(엔티티, 컴포넌트)를 관리하는 순수 데이터 클래스
 */
class SE_CORE_API World final
{
    friend class ECSRegistry;

public:
    class EntityChain;

    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

public:
    /** 모든 엔티티, 컴포넌트, 리소스를 제거하고 초기 상태로 되돌립니다. */
    void Reset();

public:
    /** Entity를 생성합니다. */
    template <typename... Components>
    EntityChain SpawnEntity(Components&&... comps)
    {
        const Entity new_entity = entity_manager.Create();
        alive_entities.Push(new_entity);

        EntityChain chain = { *this, new_entity };
        (AddComponent(chain, std::forward<Components>(comps)), ...);
        return chain;
    }

    /** Entity와 연결된 모든 Component를 제거합니다. */
    void DestroyEntity(Entity entity);

    /** Entity가 현재 살아있는지 확인합니다. */
    [[nodiscard]] bool IsEntityAlive(Entity entity) const { return entity_manager.IsValid(entity); }

    /** entity id(슬롯 인덱스)로부터 현재 살아있는 Entity를 복원합니다. */
    [[nodiscard]] Optional<Entity> TryResolveEntity(uint32 id) const { return entity_manager.TryResolveEntity(id); }

    /** 현재 살아있는 모든 Entity를 반환합니다. */
    [[nodiscard]] const Array<Entity>& GetAliveEntities() const { return alive_entities; }

public:
    /** Entity에 Component를 추가합니다. 이미 존재하면 덮어씌워집니다. */
    template <typename ComponentType>
    ComponentType& AddComponent(Entity entity, ComponentType&& init_component)
    {
        auto& storage = GetOrCreateSparseSet<ComponentType>();
        storage.Add(entity, std::forward<ComponentType>(init_component));
        return storage.Get(entity);
    }

    template <typename ComponentType, typename... Args>
    ComponentType& AddComponent(Entity entity, Args&&... args)
    {
        auto& storage = GetOrCreateSparseSet<ComponentType>();
        storage.Add(entity, ComponentType{ std::forward<Args>(args)... });
        return storage.Get(entity);
    }

    /** Entity에서 Component를 제거합니다. */
    template <typename ComponentType>
    void RemoveComponent(Entity entity)
    {
        if (const auto storage = FindSparseSet<ComponentType>())
        {
            storage->Remove(entity);
        }
    }

    /** Entity에서 ComponentType에 맞는 Component를 참조로 가져옵니다. */
    template <typename ComponentType, typename Self>
    traits::CopyConst<Self, ComponentType&> GetComponent(this Self&& self, Entity entity)
    {
        return self.template FindSparseSet<ComponentType>()->Get(entity);
    }

    /** Entity에서 ComponentType에 맞는 Component를 Optional로 가져옵니다. */
    template <typename ComponentType, typename Self>
    Optional<traits::CopyConst<Self, ComponentType&>> TryGetComponent(this Self&& self, Entity entity)
    {
        if (const auto storage = self.template FindSparseSet<ComponentType>())
        {
            return storage->Find(entity);
        }
        return NullOpt;
    }

    /** Entity가 특정 Component를 가지고 있는지 확인합니다. */
    template <typename ComponentType, typename Self>
    [[nodiscard]] bool HasComponent(this Self&& self, Entity entity)
    {
        if (auto storage = self.template FindSparseSet<ComponentType>())
        {
            return storage->Contains(entity);
        }
        return false;
    }

public:
    /**
     * Entity를 쿼리하는 Query 객체를 생성합니다.
     * @tparam Ts Component 목록 및 필터(With<...>, Without<...>)
     */
    template <typename... Ts, typename Self>
    [[nodiscard]] Query<Ts...> CreateQuery(this Self&& self)
    {
        static constexpr bool is_const_world = std::is_const_v<std::remove_reference_t<Self>>;
        static constexpr bool is_read_only_query = detail::IsReadOnlyQueryPack<Ts...>;

        static_assert(
            !is_const_world || is_read_only_query,
            "Cannot create a mutable Query from a const World. All queried components must be read-only (e.g., 'const T&')."
        );

        return Query<Ts...>{ self };
    }

public:
    /** 리소스를 삽입합니다. 이미 존재하면 값을 갱신합니다. */
    template <typename T, typename... Args>
    T& InsertResource(Args&&... args)
    {
        using RawType = std::remove_cvref_t<T>;
        const TypeId type_id = TypeId::Get<RawType>();

        if (const auto existing = resource_storages.Find(type_id))
        {
            RawType& value = static_cast<ResourceStorage<RawType>*>(existing->get())->Get();
            value = RawType{ std::forward<Args>(args)... };
            return value;
        }

        auto storage = std::make_unique<ResourceStorage<RawType>>(std::forward<Args>(args)...);
        RawType& value = storage->Get();
        resource_storages.Insert(type_id, std::move(storage));
        return value;
    }

    /** 리소스를 제거합니다. */
    template <typename T>
    void RemoveResource()
    {
        resource_storages.Remove(TypeId::Get<std::remove_cvref_t<T>>());
    }

    /** 리소스를 가져옵니다. 존재하지 않으면 Assert합니다. */
    template <typename T, typename Self>
    traits::CopyConst<Self, std::remove_cvref_t<T>&> GetResource(this Self&& self)
    {
        using RawType = std::remove_cvref_t<T>;
        const TypeId type_id = TypeId::Get<RawType>();
        const auto resource_opt = self.resource_storages.Find(type_id);
        SE_ASSERT(resource_opt.HasValue(), "Resource '{}' not found in World.", type_id.GetName());
        return static_cast<traits::CopyConst<Self, ResourceStorage<RawType>*>>(resource_opt->get())->Get();
    }

    /** 리소스를 가져오려고 시도합니다. 존재하지 않으면 NullOpt를 반환합니다. */
    template <typename T, typename Self>
    Optional<traits::CopyConst<Self, std::remove_cvref_t<T>&>> TryGetResource(this Self&& self)
    {
        using RawType = std::remove_cvref_t<T>;
        if (auto resource = self.resource_storages.Find(TypeId::Get<RawType>()))
        {
            return static_cast<traits::CopyConst<Self, ResourceStorage<RawType>*>>(resource->get())->Get();
        }
        return NullOpt;
    }

    /** 리소스가 존재하는지 확인합니다. */
    template <typename T>
    [[nodiscard]] bool HasResource() const
    {
        return resource_storages.Contains(TypeId::Get<std::remove_cvref_t<T>>());
    }

public:
    /** Schedule에서 현재 활성 CommandBuffer를 설정합니다. */
    void SetActiveCommandBuffer(CommandBuffer* buffer) { active_command_buffer = buffer; }

    /** 현재 활성 CommandBuffer를 반환합니다. */
    [[nodiscard]] CommandBuffer* GetActiveCommandBuffer() const { return active_command_buffer; }

    /**
     * 주어진 TypeId에 해당하는 IComponentStorage 포인터를 반환합니다.
     * @param type_id 검색할 타입의 TypeId
     * @return IStorage 포인터, 해당 타입이 없을 경우 nullptr 반환
     */
    [[nodiscard]] IComponentStorage* FindRawStorage(const TypeId& type_id);
    [[nodiscard]] const IComponentStorage* FindRawStorage(const TypeId& type_id) const;

    /** template 타입에 맞는 IComponentStorage 포인터를 반환합니다. */
    template <typename ComponentType, typename Self>
    traits::CopyConst<Self, IComponentStorage*> FindRawStorage(this Self&& self)
    {
        using RawType = std::remove_cvref_t<ComponentType>;
        return self.FindRawStorage(TypeId::Get<RawType>());
    }

    /** 구체적 타입으로 캐스팅된 실제 Component Pool(SparseSet)을 검색합니다. */
    template <typename ComponentType, typename Self>
    Optional<traits::CopyConst<Self, SparseSet<std::remove_cvref_t<ComponentType>>&>> FindSparseSet(this Self&& self)
    {
        using RawType = std::remove_cvref_t<ComponentType>;
        if (traits::CopyConst<Self, IComponentStorage*> storage = self.template FindRawStorage<RawType>())
        {
            return static_cast<traits::CopyConst<Self, ComponentStorage<RawType>*>>(storage)->GetStorage();
        }
        return NullOpt;
    }

    /**
     * 주어진 TypeId에 해당하는 IComponentStorage 포인터를 반환하거나,
     * 해당 타입의 저장소가 없을 경우 새로 생성합니다.
     * @param type_id 검색하거나 생성할 타입의 TypeId
     * @return IStorage 포인터, 생성되거나 조회된 저장소를 반환
     */
    [[nodiscard]] IComponentStorage* GetOrCreateRawStorage(const TypeId& type_id);

    template <typename ComponentType>
    [[nodiscard]] IComponentStorage* GetOrCreateRawStorage()
    {
        using RawType = std::remove_cvref_t<ComponentType>;
        return GetOrCreateRawStorage(TypeId::Get<RawType>());
    }

private:
    /** 타입 매개변수에 맞는 ComponentStorage 래퍼 객체를 반환하거나 생성합니다. (ECSRegistry 등에서 사용) */
    template <typename ComponentType>
    ComponentStorage<std::remove_cvref_t<ComponentType>>& GetOrCreateComponentStorage()
    {
        using RawType = std::remove_cvref_t<ComponentType>;
        const auto type_id = TypeId::Get<RawType>();

        auto& storage_ptr = component_storages
            .Entry(type_id)
            .OrInsertWith([]{ return std::make_unique<ComponentStorage<RawType>>(); });

        return *static_cast<ComponentStorage<RawType>*>(storage_ptr.get());
    }

    /** 타입 매개변수에 맞는 SparseSet을 가져오거나, 없으면 생성합니다. */
    template <typename ComponentType>
    SparseSet<std::remove_cvref_t<ComponentType>>& GetOrCreateSparseSet()
    {
        return GetOrCreateComponentStorage<ComponentType>().GetStorage();
    }

    friend SE_CORE_API void Serialize(Archive& ar, World& world);

public:
    class EntityChain
    {
    public:
        EntityChain(World& in_world, Entity new_entity)
            : world(in_world)
            , entity(new_entity)
        {
        }

        /** Entity에 Component를 추가합니다. 이미 존재하면 덮어씌워집니다. */
        template <typename ComponentType>
        EntityChain& AddComponent(ComponentType&& init_component)
        {
            world.AddComponent(entity, std::forward<ComponentType>(init_component));
            return *this;
        }

        template <typename ComponentType, typename... Args>
        EntityChain& AddComponent(Args&&... args)
        {
            world.AddComponent<ComponentType>(entity, std::forward<Args>(args)...);
            return *this;
        }

        operator Entity() const { return entity; }

    private:
        World& world;
        Entity entity;
    };

private:
    EntityManager entity_manager;
    Array<Entity> alive_entities;

    HashMap<TypeId, std::unique_ptr<IComponentStorage>> component_storages;
    HashMap<TypeId, std::unique_ptr<IResourceStorage>> resource_storages;

    CommandBuffer* active_command_buffer = nullptr;
};
} // namespace se
