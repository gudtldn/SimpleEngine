export module SimpleEngine.Core:ECS.World;
import :ECS.Entity;
import :ECS.EntityManager;
import :ECS.SparseSet;
import :ECS.QueryResult;

import SimpleEngine.Types;
import std;


namespace se::core::ecs
{
/** 엔진이 동시에 관리할 수 있는 최대 엔티티 개수 */
constexpr uint32 MAX_ENTITIES = 65536;

struct IStorage
{
    virtual ~IStorage() = default;
    virtual void Remove(Entity entity) = 0;
};

template <typename ComponentType>
struct ComponentStorage : IStorage
{
    SparseSet<ComponentType> storage;

    ComponentStorage(size_t max_entities)
        : storage(max_entities)
    {
    }

    virtual void Remove(Entity entity) override { storage.Remove(entity); }
};

/**
 * ECS 월드의 모든 요소(엔티티, 컴포넌트)를 관리하는 중앙 클래스
 */
export class World final
{
public:
    class EntityChain;

    World()
        : entity_manager(MAX_ENTITIES)
    {
    }

    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

public:
    /** World에 새로운 Entity를 생성합니다. */
    EntityChain CreateEntity()
    {
        return { this, entity_manager.Create() };
    }

    /** Entity와 Entity와 연결된 Component를 제거합니다. */
    void DestroyEntity(Entity entity)
    {
        for (const auto& storage : component_storages | std::views::values)
        {
            storage->Remove(entity);
        }
        entity_manager.Destroy(entity);
    }

    /** Entity에 Component를 추가합니다. 만약 이미 존재하면 덮어씌워집니다. */
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

    template <typename... Components>
    QueryResult<Components...> Query()
    {
        // TODO: Implements this
        return {};
    }

private:
    template <typename ComponentType>
    SparseSet<ComponentType>& GetOrCreateStorage()
    {
        const auto type_index = std::type_index(typeid(ComponentType));
        if (!component_storages.contains(type_index))
        {
            component_storages[type_index] =
                std::make_unique<ComponentStorage<ComponentType>>(entity_manager.GetMaxEntities());
        }

        ComponentStorage<ComponentType>* wrapper =
            static_cast<ComponentStorage<ComponentType>*>(component_storages.at(type_index).get());

        return wrapper->storage;
    }

    template <typename ComponentType>
    auto GetStorage(this auto&& self)
    {
        // self의 cv-qualifier에 따라 const/비-const 자동 추론
        using SelfType = std::remove_reference_t<decltype(self)>;
        using StorageType = std::conditional_t<
            std::is_const_v<SelfType>,
            const SparseSet<ComponentType>&,
            SparseSet<ComponentType>&
        >;

        const auto type_index = std::type_index(typeid(ComponentType));
        if (self.component_storages.contains(type_index))
        {
            return Optional<StorageType>{
                static_cast<ComponentStorage<ComponentType>*>(self.component_storages.at(type_index).get())->storage
            };
        }
        return Optional<StorageType>{};
    }

private:
    EntityManager entity_manager;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> component_storages;

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
