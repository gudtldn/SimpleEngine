export module SE.Core:ECS.Entity;

import SE.Types;
import std;


namespace se::core::ecs
{
export class Entity
{
public:
    uint32 GetId() const noexcept { return id; }
    uint32 GetGeneration() const noexcept { return generation; }

    bool operator==(const Entity& other) const noexcept = default;
    bool operator!=(const Entity& other) const noexcept = default;

private:
    friend class EntityManager;

    Entity(uint32 in_id, uint32 in_generation) noexcept
        : id(in_id)
        , generation(in_generation)
    {
    }

    uint32 id;
    uint32 generation;
};
}
