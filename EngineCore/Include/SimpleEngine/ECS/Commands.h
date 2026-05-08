#pragma once

#include "SimpleEngine/ECS/CommandBuffer.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/ECS/World.h"

#include <utility>


namespace se
{
/**
 * 특정 Entity에 대한 지연 명령을 체이닝 방식으로 구성하는 빌더
 */
class EntityCommands
{
public:
    EntityCommands(CommandBuffer& in_buffer, Entity in_entity)
        : buffer(in_buffer)
        , entity(in_entity)
    {
    }

    /** 컴포넌트를 추가합니다. 이미 존재하면 덮어씌워집니다. */
    template <typename ComponentType>
    EntityCommands& Insert(ComponentType&& component)
    {
        buffer.Push([e = entity, comp = std::forward<ComponentType>(component)](World& world) mutable
        {
            world.AddComponent(e, std::move(comp));
        });
        return *this;
    }

    /** 컴포넌트를 제거합니다. */
    template <typename ComponentType>
    EntityCommands& Remove()
    {
        buffer.Push([e = entity](World& world)
        {
            world.RemoveComponent<ComponentType>(e);
        });
        return *this;
    }

    /** Entity를 제거합니다. */
    void Despawn()
    {
        buffer.Push([e = entity](World& world)
        {
            world.DestroyEntity(e);
        });
    }

private:
    CommandBuffer& buffer;
    Entity entity;
};

/**
 * 시스템 내에서 World 구조 변경을 안전하게 지연 실행하기 위한 래퍼
 *
 * 시스템 파라미터로 주입되며, 실행 중 큐에 명령을 쌓고
 * 시스템 실행이 끝난 후 일괄 적용됩니다.
 *
 * @code
 * void MySystem(Commands commands, Query<Entity, const Health&> query)
 * {
 *     for (auto [entity, health] : query)
 *     {
 *         if (health.hp <= 0)
 *             commands.Entity(entity).Despawn();
 *     }
 * }
 * @endcode
 */
class Commands
{
public:
    explicit Commands(CommandBuffer& in_buffer)
        : buffer(in_buffer)
    {
    }

    /** 새 Entity를 생성합니다. */
    template <typename... Components>
    void Spawn(Components&&... comps)
    {
        buffer.Push([...comps = std::forward<Components>(comps)](World& world) mutable
        {
            world.SpawnEntity(std::move(comps)...);
        });
    }

    /** 특정 Entity에 대한 명령 빌더를 반환합니다. */
    EntityCommands Entity(Entity entity) { return { buffer, entity }; }

    /** 리소스를 삽입합니다. */
    template <typename T, typename... Args>
    void InsertResource(Args&&... args)
    {
        buffer.Push([...args = std::forward<Args>(args)](World& world) mutable
        {
            world.InsertResource<T>(std::move(args)...);
        });
    }

    /** 리소스를 제거합니다. */
    template <typename T>
    void RemoveResource()
    {
        buffer.Push([](World& world)
        {
            world.RemoveResource<T>();
        });
    }

private:
    CommandBuffer& buffer;
};
} // namespace se
