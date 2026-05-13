#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Utility/HashUtils.h"


namespace se
{
class EntityManager;

class Entity
{
public:
    static constexpr u32 INVALID_ID = std::numeric_limits<u32>::max();

public:
    Entity()
        : id(INVALID_ID)
        , generation(0)
    {
    }

    [[nodiscard]] u32 GetId() const noexcept { return id; }
    [[nodiscard]] u32 GetGeneration() const noexcept { return generation; }

    [[nodiscard]] bool IsValid() const noexcept { return id != INVALID_ID; }

    bool operator==(const Entity& other) const noexcept = default;
    [[nodiscard]] explicit operator bool() const noexcept { return IsValid(); }

private:
    friend class EntityManager;

    friend void Serialize(Archive& ar, Entity& entity)
    {
        ar("id") << entity.id;
        ar("generation") << entity.generation;
    }

    Entity(u32 in_id, u32 in_generation) noexcept
        : id(in_id)
        , generation(in_generation)
    {
    }

    u32 id;
    u32 generation;
};
} // namespace se

template <>
struct std::hash<se::Entity>
{
    usize operator()(const se::Entity& entity) const noexcept
    {
        usize hash = 0;
        se::HashUtils::Combine(hash, entity.GetId(), entity.GetGeneration());
        return static_cast<usize>(hash);
    }
};
