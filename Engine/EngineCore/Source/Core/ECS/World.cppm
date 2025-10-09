export module SE.Core:ECS.World;
import :ECS.Entity;
import :ECS.EntityManager;
import :ECS.SparseSet;
import :ECS.Schedules;
import :Function;

import SE.Types;
import SE.Traits;
import SE.Utility;
import std;

using namespace se::core::ecs::schedules;
using namespace se::traits::type_traits;
using namespace se::traits::func_traits;
using namespace se::utility::type;


namespace
{
template <typename Fn>
concept SystemFuncType = IsFunctionType<Fn> && std::is_void_v<typename FunctionTraits<Fn>::ReturnType>;
}

export namespace se::core::ecs
{
class World;

/**
 * ECS 월드의 모든 요소(엔티티, 컴포넌트)를 관리하는 중앙 클래스
 */
class World final
{
private:
    template <typename... Ts>
        requires (sizeof...(Ts) > 0)
        && TupleHasUniqueTypes<FlattenTuple<std::tuple<Ts...>>>
    friend class Query;

    EntityManager entity_manager;
    // TODO: 추후 C++26에서 Annotation으로 Tag 검사
    unordered_map<std::type_index, vector<function::Function<void()>>> systems;
    unordered_map<std::type_index, std::unique_ptr<IStorage>> component_storages;

public:
    class EntityChain;

    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

public:
    /** World에 새로운 Entity를 생성합니다. */
    EntityChain CreateEntity();

    /** Entity와 Entity와 연결된 Component를 제거합니다. */
    void DestroyEntity(Entity entity);

    /** 현재 살아있는 모든 Entity를 반환합니다. */
    vector<Entity> GetAliveEntities() const;

    /** Entity에 Component를 추가합니다. 만약 이미 존재하면 덮어씌워집니다. */
    template <typename ComponentType>
    ComponentType& AddComponent(Entity entity, ComponentType&& init_component)
    {
        auto& storage = GetOrCreateStorage<ComponentType>();
        storage.Add(entity, std::move(init_component));
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

    template <ScheduleType S, SystemFuncType Fn>
    void AddSystem(Fn&& system_func)
    {
        const std::type_index idx = std::type_index(typeid(S));
        systems[idx].push_back([this, sys_func = std::forward<Fn>(system_func)] mutable
        {
            using F = FunctionTraits<Fn>;
            auto tuple = utility::type::WithUnpackedTypes<typename F::ArgumentTypes>([this]<typename... Ts>
            {
                return std::make_tuple(CreateSystemParam<Ts>()...);
            });
            std::apply(sys_func, tuple);
        });
    }

public:
    /** */
    template <ScheduleType S>
    void RunSchedule()
    {
        const std::type_index idx = std::type_index(typeid(S));
        if (const auto it = systems.find(idx); it != systems.end())
        {
            for (const function::Function<void()>& system : it->second)
            {
                system();
            }
        }
    }

private:
    template <typename T>
    T CreateSystemParam()
    {
        using namespace traits::type_traits;
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
            std::unreachable();
        }
    }

private:
    template <typename ComponentType>
    SparseSet<ComponentType>& GetOrCreateStorage()
    {
        const auto type_index = std::type_index(typeid(ComponentType));
        if (!component_storages.contains(type_index))
        {
            component_storages[type_index] = std::make_unique<ComponentStorage<ComponentType>>();
        }
        ComponentStorage<ComponentType>* wrapper = static_cast<ComponentStorage<ComponentType>*>(component_storages.at(type_index).get());
        return wrapper->GetStorage();
    }

    template <typename ComponentType>
    Optional<SparseSet<ComponentType>&> GetStorage()
    {
        if (IStorage* storage = GetIStorage<ComponentType>())
        {
            return static_cast<ComponentStorage<ComponentType>*>(storage)->GetStorage();
        }
        return std::nullopt;
    }

    template <typename ComponentType>
    Optional<const SparseSet<ComponentType>&> GetStorage() const
    {
        if (const IStorage* storage = GetIStorage<ComponentType>())
        {
            return static_cast<const ComponentStorage<ComponentType>*>(storage)->GetStorage();
        }
        return std::nullopt;
    }

    /** 타입에 맞는 IStorage 포인터를 반환합니다. 쿼리 시스템 내부에서 사용됩니다. */
    template <typename ComponentType>
    [[nodiscard]] IStorage* GetIStorage()
    {
        const auto type_index = std::type_index(typeid(ComponentType));
        if (component_storages.contains(type_index))
        {
            return component_storages.at(type_index).get();
        }
        return nullptr;
    }

    /** 타입에 맞는 IStorage 포인터를 반환합니다. 쿼리 시스템 내부에서 사용됩니다. */
    template <typename ComponentType>
    [[nodiscard]] const IStorage* GetIStorage() const
    {
        const auto type_index = std::type_index(typeid(ComponentType));
        if (component_storages.contains(type_index))
        {
            return component_storages.at(type_index).get();
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

        /** Entity에 Component를 추가합니다. 만약 이미 존재하면 덮어씌워집니다. */
        template <typename ComponentType>
        EntityChain& AddComponent(ComponentType&& init_component)
        {
            world->AddComponent<ComponentType>(entity, std::move(init_component));
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
}
