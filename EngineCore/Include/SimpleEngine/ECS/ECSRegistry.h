#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/ECS/World.h"

#include <concepts>


namespace se
{
/**
 * 컴포넌트 타입에 구애받지 않고 ECS 작업을 수행하기 위한 Operations
 */
struct ComponentOps
{
    /**
     * ComponentStorage를 초기화합니다.
     * @param world 작업을 수행할 World
     */
    IComponentStorage* (*ensure_storage)(World& world);

    /**
     * Entity에 컴포넌트를 추가합니다. (기본 생성자 호출)
     * @param world 작업을 수행할 World
     * @param entity 컴포넌트를 추가할 Entity
     */
    void (*add_component)(World& world, Entity entity);

    /**
     * Entity에서 컴포넌트를 삭제합니다.
     * @param world 작업을 수행할 World
     * @param entity 컴포넌트를 삭제할 Entity
     */
    void (*remove_component)(World& world, Entity entity);

    /**
     * Entity의 컴포넌트 보유 여부를 확인합니다.
     * @param world 작업을 수행할 World
     * @param entity 확인할 Entity
     * @return 컴포넌트 보유 여부 (bool)
     */
    bool (*has_component)(const World& world, Entity entity);

    /**
     * 컴포넌트의 읽기 전용 포인터를 획득합니다.
     * @param world 작업을 수행할 World
     * @param entity 확인할 Entity
     * @return 컴포넌트의 상수 포인터 (const void*)
     */
    const void* (*get_component)(const World& world, Entity entity);

    /**
     * 컴포넌트의 수정 가능한 포인터를 획득합니다.
     * @param world 작업을 수행할 World
     * @param entity 확인할 Entity
     * @return 컴포넌트의 포인터 (void*)
     */
    void* (*get_component_mutable)(World& world, Entity entity);
};


/** 리소스 타입에 구애받지 않고 ECS 작업을 수행하기 위한 Operations */
struct ResourceOps
{
    /**
     * World에 리소스를 추가합니다. (기본 생성자 호출)
     * @param world 작업을 수행할 World
     */
    void (*insert_default)(World& world);

    /**
     * World에서 리소스를 삭제합니다.
     * @param world 작업을 수행할 World
     */
    void (*remove_resource)(World& world);

    /**
     * World의 리소스 보유 여부를 확인합니다.
     * @param world 확인할 World
     * @return 리소스 보유 여부 (bool)
     */
    bool (*has_resource)(const World& world);

    /**
     * 리소스의 읽기 전용 포인터를 획득합니다.
     * @param world 확인할 World
     * @return 리소스의 상수 포인터 (const void*)
     */
    const void* (*get_resource)(const World& world);

    /**
     * 리소스의 수정 가능한 포인터를 획득합니다.
     * @param world 작업을 수행할 World
     * @return 리소스의 포인터 (void*)
     */
    void* (*get_resource_mutable)(World& world);
};

/**
 * 리플렉션 및 에디터가 타입 정보 없이 컴포넌트를 조작할 수 있도록 타입별 Operations를 등록하고 관리하는 싱글톤 레지스트리
 */
class SE_CORE_API ECSRegistry
{
    ECSRegistry() = default;

public:
    ECSRegistry(const ECSRegistry&) = delete;
    ECSRegistry& operator=(const ECSRegistry&) = delete;
    ECSRegistry(ECSRegistry&&) = delete;
    ECSRegistry& operator=(ECSRegistry&&) = delete;
    ~ECSRegistry() = default;

    [[nodiscard]] static ECSRegistry& Get();

public:
    /** 컴포넌트 T에 대한 ComponentOps를 등록합니다. */
    template <typename T>
    void RegisterComponentOps()
    {
        static_assert(std::default_initializable<T>, "Component T must be default constructible.");
        static_assert(std::move_constructible<T>, "Component T must be move constructible.");

        const TypeId type_id = TypeId::Get<T>();
        SE_ASSERT(!component_operators.Contains(type_id), "Component '{}' is already registered! Check your initialization logic.", type_id.GetName());

        component_operators.Insert(type_id, ComponentOps{
            .ensure_storage = [](World& world) static -> IComponentStorage*
            {
                return &world.GetOrCreateComponentStorage<T>();
            },
            .add_component = [](World& world, Entity entity) static
            {
                world.AddComponent<T>(entity);
            },
            .remove_component = [](World& world, Entity entity) static
            {
                world.RemoveComponent<T>(entity);
            },
            .has_component = [](const World& world, Entity entity) static -> bool
            {
                return world.HasComponent<T>(entity);
            },
            .get_component = [](const World& world, Entity entity) static -> const void*
            {
                if (Optional comp_opt = world.TryGetComponent<T>(entity))
                {
                    return &comp_opt.Value();
                }
                return nullptr;
            },
            .get_component_mutable = [](World& world, Entity entity) static -> void*
            {
                if (Optional comp_opt = world.TryGetComponent<T>(entity))
                {
                    return &comp_opt.Value();
                }
                return nullptr;
            }
        });
    }

    /** 리소스 T에 대한 ResourceOps를 등록합니다. */
    template <typename T>
    void RegisterResourceOps()
    {
        static_assert(std::default_initializable<T>, "Resource T must be default constructible.");
        static_assert(std::move_constructible<T>, "Resource T must be move constructible.");

        const TypeId type_id = TypeId::Get<T>();
        SE_ASSERT(!resource_operators.Contains(type_id), "Resource '{}' is already registered! Check your initialization logic.", type_id.GetName());

        resource_operators.Insert(type_id, ResourceOps{
            .insert_default = [](World& world) static
            {
                world.InsertResource<T>();
            },
            .remove_resource = [](World& world) static
            {
                world.RemoveResource<T>();
            },
            .has_resource = [](const World& world) static -> bool
            {
                return world.HasResource<T>();
            },
            .get_resource = [](const World& world) -> const void*
            {
                if (const auto resource = world.TryGetResource<T>())
                {
                    return &resource.Value();
                }
                return nullptr;
            },
            .get_resource_mutable = [](World& world) -> void*
            {
                if (const auto resource = world.TryGetResource<T>())
                {
                    return &resource.Value();
                }
                return nullptr;
            },
        });
    }

    /** 해당 컴포넌트 타입의 Ops를 찾습니다. */
    [[nodiscard]] Optional<const ComponentOps&> GetComponentOps(const TypeId& type_id) const;

    /** 해당 리소스 타입의 Ops를 찾습니다. */
    [[nodiscard]] Optional<const ResourceOps&> GetResourceOps(const TypeId& type_id) const;

    /** 등록된 모든 컴포넌트 타입의 Ops 맵을 반환합니다. */
    [[nodiscard]] const HashMap<TypeId, ComponentOps>& GetComponentOpsMap() const { return component_operators; }

    /** 등록된 모든 리소스 타입의 Ops 맵을 반환합니다. */
    [[nodiscard]] const HashMap<TypeId, ResourceOps>& GetResourceOpsMap() const { return resource_operators; }

private:
    HashMap<TypeId, ComponentOps> component_operators;
    HashMap<TypeId, ResourceOps> resource_operators;
};
} // namespace se
