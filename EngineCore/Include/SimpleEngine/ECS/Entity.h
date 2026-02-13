#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Utility/HashUtils.h"


namespace se
{
namespace ecs
{
class EntityManager;
}

class Entity
{
public:
    static constexpr uint32 InvalidId = std::numeric_limits<uint32>::max();

public:
    Entity()
        : id(InvalidId)
        , generation(0)
    {
    }

    [[nodiscard]] uint32 GetId() const noexcept { return id; }
    [[nodiscard]] uint32 GetGeneration() const noexcept { return generation; }

    [[nodiscard]] bool IsValid() const noexcept { return id != InvalidId; }

    bool operator==(const Entity& other) const noexcept = default;
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

private:
    friend class se::ecs::EntityManager;

    friend void Serialize(Archive& ar, Entity& entity)
    {
        ar("id") << entity.id;
        ar("generation") << entity.generation;
    }

    Entity(uint32 in_id, uint32 in_generation) noexcept
        : id(in_id)
        , generation(in_generation)
    {
    }

    uint32 id;
    uint32 generation;
};
}  // namespace se

template <>
struct std::hash<se::Entity>
{
    size_t operator()(const se::Entity& entity) const noexcept
    {
        usize hash = 0;
        se::HashUtils::Combine(hash, entity.GetId(), entity.GetGeneration());
        return static_cast<size_t>(hash);
    }
};
