#pragma once
#include <type_traits>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::ecs
{
/**
 * 컴포넌트 타입에 구애받지 않고 ECS 작업을 수행하기 위한 인터페이스
 */
struct ComponentInterface
{
    /**
     * ComponentStorage를 초기화합니다.
     * @param world 작업을 수행할 World
     */
    IStorage* (*ensure_storage)(World& world);

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
    [[nodiscard]] bool (*has_component)(const World& world, Entity entity);

    /**
     * 컴포넌트의 읽기 전용 포인터를 획득합니다.
     * @param world 작업을 수행할 World
     * @param entity 확인할 Entity
     * @return 컴포넌트의 상수 포인터 (const void*)
     */
    [[nodiscard]] const void* (*get_component)(const World& world, Entity entity);

    /**
     * 컴포넌트의 수정 가능한 포인터를 획득합니다.
     * @param world 작업을 수행할 World
     * @param entity 확인할 Entity
     * @return 컴포넌트의 포인터 (void*)
     */
    [[nodiscard]] void* (*get_component_mutable)(World& world, Entity entity);
};

/**
 * @todo docs
 */
class SE_CORE_API ComponentRegistry
{
    ComponentRegistry() = default;

public:
    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;
    ComponentRegistry(const ComponentRegistry&&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&&) = delete;
    ~ComponentRegistry() = default;

    [[nodiscard]] static ComponentRegistry& Get();

public:
    /** 컴포넌트 T에 대한 ComponentInterface를 등록합니다. */
    template <typename T>
    void RegisterInterface()
    {
        static_assert(std::is_default_constructible_v<T>, "Component T must be default constructible.");
        static_assert(std::is_move_constructible_v<T>, "Component T must be move constructible.");

        const TypeId type_id = TypeId::Get<T>();
        SE_ASSERT(!interfaces.Contains(type_id), "Component '{}' is already registered! Check your initialization logic.", type_id.GetName());

        interfaces.Insert(type_id, ComponentInterface{
            .ensure_storage = [](World& world) static -> IStorage*
            {
                return &world.GetOrCreateStorageImpl<T>();
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
                if (const Optional comp_opt = world.TryGetComponent<T>(entity))
                {
                    return &comp_opt.Value();
                }
                return nullptr;
            },
            .get_component_mutable = [](World& world, Entity entity) static -> void*
            {
                if (const Optional comp_opt = world.TryGetComponent<T>(entity))
                {
                    return &comp_opt.Value();
                }
                return nullptr;
            }
        });
    }

    /** 해당 타입의 Interface를 찾습니다. */
    [[nodiscard]] Optional<const ComponentInterface&> GetInterface(const TypeId& type_id) const;

private:
    HashMap<TypeId, ComponentInterface> interfaces;
};
}  // namespace se::ecs
