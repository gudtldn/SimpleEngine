#pragma once
#include <concepts>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"
#include "SimpleEngine/ECS/EntityManager.h"
#include "SimpleEngine/ECS/QueryConcepts.h"
#include "SimpleEngine/ECS/Schedules.h"
#include "SimpleEngine/ECS/SparseSet.h"


namespace se::ecs
{
template <typename... Ts>
    requires QueryParameterPack<Ts...>
class Query;


namespace details
{
template <typename Fn>
concept SystemFuncType = traits::IsFunctionType<Fn>
    && std::is_void_v<typename traits::FunctionTraits<Fn>::ReturnType>;
}

/**
 * ECS 월드의 모든 요소(엔티티, 컴포넌트)를 관리하는 중앙 클래스
 */
class SE_CORE_API World final
{
private:
    template <typename... Ts>
    friend class QueryData;

    friend class ComponentRegistry;

    EntityManager entity_manager;
    // TODO: 추후 C++26에서 Annotation으로 Tag 검사
    HashMap<refl::TypeId, Array<Function<void()>>> systems;
    HashMap<refl::TypeId, std::unique_ptr<IStorage>> component_storages;

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
        EntityChain entity = { this, entity_manager.Create() };
        (AddComponent(entity, std::forward<Components>(comps)), ...);
        return entity;
    }

    /** Entity와 Entity와 연결된 Component를 제거합니다. */
    void DestroyEntity(Entity entity);

    /** 현재 살아있는 모든 Entity를 반환합니다. */
    [[nodiscard]] Array<Entity> GetAliveEntities() const;

    /** Entity에 Component를 추가합니다. 만약 이미 존재하면 덮어씌워집니다. */
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
    template <typename ComponentType>
    ComponentType& GetComponent(Entity entity)
    {
        return GetStorage<ComponentType>()->Get(entity);
    }

    /** Entity에서 ComponentType에 맞는 Component를 참조로 가져옵니다. */
    template <typename ComponentType>
    const ComponentType& GetComponent(Entity entity) const
    {
        return GetStorage<ComponentType>()->Get(entity);
    }

    /** Entity에서 ComponentType에 맞는 Component를 포인터로 가져옵니다. */
    template <typename ComponentType>
    Optional<ComponentType&> TryGetComponent(Entity entity)
    {
        if (Optional opt_storage = GetStorage<ComponentType>())
        {
            return opt_storage->TryGet(entity);
        }
        return std::nullopt;
    }

    /** Entity에서 ComponentType에 맞는 Component를 포인터로 가져옵니다. */
    template <typename ComponentType>
    Optional<const ComponentType&> TryGetComponent(Entity entity) const
    {
        if (Optional opt_storage = GetStorage<ComponentType>())
        {
            return opt_storage->TryGet(entity);
        }
        return std::nullopt;
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

    /**
     * 지정된 스케줄 단계에 시스템을 추가합니다.
     * @details 시스템은 함수 시그니처를 분석하여 필요한 자원(Query, World* 등)을 자동으로 주입받습니다.
     * @tparam S 시스템을 추가할 스케줄 타입 (예: PreUpdate, Update, PostUpdate)
     * @tparam Fn 시스템으로 등록할 함수 또는 람다
     */
    template <schedule::ScheduleType S, details::SystemFuncType Fn>
    void AddSystem(Fn&& system_func)
    {
        const auto type_id = refl::TypeId::Get<S>();
        systems[type_id].Push([this, sys_func = std::forward<Fn>(system_func)] mutable
        {
            using F = traits::FunctionTraits<Fn>;
            std::tuple tuple = utility::WithUnpackedTypes<typename F::ArgumentTypes>([this]<typename... Ts>
            {
                return std::make_tuple(CreateSystemParam<Ts>()...);
            });
            std::apply(sys_func, std::move(tuple));
        });
    }

public:
    /**
     * 주어진 TypeId에 해당하는 IStorage 포인터를 반환합니다.
     * @param type_id 검색할 타입의 TypeId
     * @return IStorage 포인터, 해당 타입이 없을 경우 nullptr 반환
     */
    IStorage* GetStorage(const refl::TypeId& type_id);

    /**
     * 지정된 스케줄에 등록된 모든 시스템을 순서대로 실행합니다.
     * @tparam S 실행할 스케줄 타입
     */
    template <schedule::ScheduleType S>
    void RunSchedule()
    {
        const auto type_id = refl::TypeId::Get<S>();
        if (Optional system_opt = systems.Find(type_id))
        {
            for (const Function<void()>& system : *system_opt)
            {
                system();
            }
        }
    }

    /**
     * Entity를 쿼리하는 Query 객체를 생성합니다.
     * @tparam Ts Component 목록 및 필터(With<...>, Without<...>)
     * @return Query 객체
     */
    template <typename... Ts>
        requires requires { Query<Ts...>{ std::declval<World*>() }; }
    [[nodiscard]] Query<Ts...> QueryEntities() // TODO: QueryEntities를 const로 만들기
    {
        return Query<Ts...>{ this };
    }

private:
    template <typename T>
    T CreateSystemParam()
    {
        using namespace traits;
        if constexpr (std::same_as<T, World*>)
        {
            return this;
        }
        else if constexpr (IsSpecializationOf<T, Query>)
        {
            return T{ this };
        }
        else
        {
            static_assert(AlwaysFalse<T>, "Invalid system parameter type");
            SE_UNREACHABLE();
        }
    }

private:
    template <typename ComponentType>
    SparseSet<std::decay_t<ComponentType>>& GetOrCreateStorage()
    {
        using RawType = std::decay_t<ComponentType>;

        const auto type_id = refl::TypeId::Get<RawType>();
        IStorage* storage = component_storages
            .Entry(type_id)
            .OrInsert(std::make_unique<ComponentStorage<RawType>>()).get();

        auto* wrapper = static_cast<ComponentStorage<RawType>*>(storage);
        return wrapper->GetStorage();
    }

    template <typename ComponentType, typename Self>
    Optional<traits::DeduceRetType<Self, SparseSet<std::decay_t<ComponentType>>&>> GetStorage(this Self&& self)
    {
        using RawType = std::decay_t<ComponentType>;
        if (traits::DeduceRetType<Self, IStorage*> storage = self.template GetIStorage<RawType>())
        {
            return static_cast<traits::DeduceRetType<Self, ComponentStorage<RawType>*>>(storage)->GetStorage();
        }
        return std::nullopt;
    }

    /** 타입에 맞는 IStorage 포인터를 반환합니다. 쿼리 시스템 내부에서 사용됩니다. */
    template <typename ComponentType, typename Self>
    traits::DeduceRetType<Self, IStorage*> GetIStorage(this Self&& self)
    {
        using RawType = std::decay_t<ComponentType>;
        std::unique_ptr<IStorage> null_ptr;

        const auto type_id = refl::TypeId::Get<RawType>();
        return self.component_storages.Find(type_id).ValueOr(null_ptr).get();
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

        /** Entity에 Component를 추가합니다. 만약 이미 존재하면 덮어씌워집니다. */
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
};
}  // namespace se::ecs
