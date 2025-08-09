export module SimpleEngine.Core:ECS.World;
import :ECS.Entity;
import :ECS.EntityManager;
import :ECS.SparseSet;

import SimpleEngine.Types;
import std;


namespace se::core::ecs
{
export class World;

/** 엔진이 동시에 관리할 수 있는 최대 엔티티 개수 */
constexpr uint32 MAX_ENTITIES = 65536;

export template <typename FetchList, typename WithList, typename WithoutList>
class QueryResult;

export template <typename... T> struct FetchQuery {};
export template <typename... T> struct WithQuery {};
export template <typename... T> struct WithoutQuery {};


/** type erasure를 위한 인터페이스 */
class IStorage
{
public:
    virtual ~IStorage() = default;

    /** Storage의 크기를 반환합니다. */
    [[nodiscard]] virtual size_t Length() const noexcept = 0;

    /** Storage가 비어있는지 확인합니다. */
    [[nodiscard]] virtual bool IsEmpty() const noexcept = 0;

    /** 해당 엔티티가 컴포넌트를 가지고 있는지 확인합니다. */
    [[nodiscard]] virtual bool Contains(Entity entity) const noexcept = 0;

    /** Index로 Entity를 가져옵니다. */
    [[nodiscard]] virtual Optional<Entity> GetEntityByIndex(size_t index) const = 0;

    /** 엔티티가 가지고 있는 컴포넌트를 제거합니다. */
    virtual void Remove(Entity entity) = 0;
};

template <typename ComponentType>
class ComponentStorage : public IStorage
{
    SparseSet<ComponentType> storage;

public:
    explicit ComponentStorage(size_t max_entities)
        : storage(max_entities)
    {
    }

    //~Begin IStorage
    [[nodiscard]] virtual size_t Length() const noexcept override { return storage.Length(); }
    [[nodiscard]] virtual bool IsEmpty() const noexcept override { return storage.IsEmpty(); }
    [[nodiscard]] virtual bool Contains(Entity entity) const noexcept override { return storage.Contains(entity); }
    [[nodiscard]] virtual Optional<Entity> GetEntityByIndex(size_t index) const override { return storage.GetEntityByIndex(index); }
    virtual void Remove(Entity entity) override { storage.Remove(entity); }
    //~End IStorage

    SparseSet<ComponentType>& GetStorage() { return storage; }
    const SparseSet<ComponentType>& GetStorage() const { return storage; }
};

/**
 * ECS 월드의 모든 요소(엔티티, 컴포넌트)를 관리하는 중앙 클래스
 * @todo World.cpp로 분리
 */
export class World final
{
private:
    template <typename FetchList, typename WithList, typename WithoutList>
    friend class QueryResult;

    EntityManager entity_manager;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> component_storages;

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

    /** TODO: docs */
    template <typename... Components>
    auto Query() { return QueryResult<FetchQuery<Components...>, WithQuery<>, WithoutQuery<>>{ this }; }

private:
    template <typename ComponentType>
    SparseSet<ComponentType>& GetOrCreateStorage()
    {
        const auto type_index = std::type_index(typeid(ComponentType));
        if (!component_storages.contains(type_index))
        {
            component_storages[type_index] = std::make_unique<ComponentStorage<ComponentType>>(entity_manager.GetMaxEntities());
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


/**
 * TODO: docs
 */
export template <
    typename... FetchComps,
    typename... WithComps,
    typename... WithoutComps
>
class QueryResult<FetchQuery<FetchComps...>, WithQuery<WithComps...>, WithoutQuery<WithoutComps...>>
{
public:
    QueryResult(World* in_world)
        : world(in_world)
    {
    }

public:
    template <typename... NewWithComps>
    auto With()
    {
        return QueryResult<
            FetchQuery<FetchComps...>,
            WithQuery<WithComps..., NewWithComps...>,
            WithoutQuery<WithoutComps...>
        >{ world };
    }

    template <typename... NewWithoutComps>
    auto Without()
    {
        return QueryResult<
            FetchQuery<FetchComps...>,
            WithQuery<WithComps...>,
            WithoutQuery<WithoutComps..., NewWithoutComps...>
        >{ world };
    }

    template <typename Fn>
        requires std::invocable<Fn, Entity, FetchComps&...>
    QueryResult& ForEach(Fn&& func)
    {
        for (auto tuple_value : *this)
        {
            std::apply(std::forward<Fn>(func), tuple_value);
        }
        return *this;
    }

public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<Entity, FetchComps&...>;
        using difference_type = std::ptrdiff_t;

    public:
        Iterator(World* in_world, IStorage* in_pool, size_t in_index)
            : world(in_world)
            , base_pool(in_pool)
            , storage_index(in_index)
        {
            AdvanceToValid();
        }

        value_type operator*() const noexcept
        {
            Entity entity = base_pool->GetEntityByIndex(storage_index).value();
            return std::tie(entity, world->GetComponent<FetchComps>(entity)...);
        }

        Iterator& operator++()
        {
            ++storage_index;
            AdvanceToValid();
            return *this;
        }

        bool operator==(const Iterator& other) const noexcept
        {
            return storage_index == other.storage_index && base_pool == other.base_pool;
        }

    private:
        void AdvanceToValid()
        {
            // 기준 풀이 없거나, 인덱스가 끝에 도달했으면 즉시 종료
            if (!base_pool || storage_index >= base_pool->Length())
            {
                return;
            }

            while (storage_index < base_pool->Length())
            {
                if (Optional<Entity> entity_opt = base_pool->GetEntityByIndex(storage_index))
                {
                    Entity entity = *entity_opt;

                    // Fetch와 With에 있는 Component를 가지고 있는지 확인
                    const bool has_all_required =
                        (world->HasComponent<FetchComps>(entity) && ...)
                        && (world->HasComponent<WithComps>(entity) && ...);

                    if (!has_all_required)
                    {
                        ++storage_index;
                        continue;
                    }

                    // Without도 가지고 있는지 검사
                    const bool has_any_excluded = (world->HasComponent<WithoutComps>(entity) || ...);
                    if (has_any_excluded)
                    {
                        ++storage_index;
                        continue;
                    }

                    // 유효한 엔티티면 return
                    return;
                }
                ++storage_index;
            }
        }

    private:
        World* world;
        IStorage* base_pool;
        size_t storage_index;
    };

    Iterator begin()
    {
        IStorage* smallest_pool = FindSmallestPool();
        return Iterator(world, smallest_pool, 0);
    }

    Iterator end()
    {
        IStorage* smallest_pool = FindSmallestPool();
        const size_t end_index = smallest_pool ? smallest_pool->Length() : 0;
        return Iterator(world, smallest_pool, end_index);
    }

private:
    /** Fetch와 With 목록의 모든 SparseSet 중 가장 작은 것을 찾아 반환합니다. */
    IStorage* FindSmallestPool()
    {
        std::array<IStorage*, sizeof...(FetchComps) + sizeof...(WithComps)> pools;
        size_t i = 0;
        ((pools[i++] = world->GetIStorage<FetchComps>()), ...);
        ((pools[i++] = world->GetIStorage<WithComps>()), ...);
        (void)i;

        if (pools.empty())
        {
            return nullptr;
        }

        auto it = std::min_element(pools.begin(), pools.end(), [](const IStorage* a, const IStorage* b)
        {
            if (!a)
            {
                return false;
            }
            if (!b)
            {
                return true;
            }
            return a->Length() < b->Length();
        });

        return *it;
    }

private:
    World* world;
};
}
