#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::world
{
class Entity
{
public:
    [[nodiscard]] uint32 GetId() const noexcept { return id; }
    [[nodiscard]] uint32 GetGeneration() const noexcept { return generation; }

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

template <>
struct std::hash<se::world::Entity>
{
    size_t operator()(const se::world::Entity& entity) const noexcept
    {
        return std::hash<uint32>{}(entity.GetId());
    }
};
