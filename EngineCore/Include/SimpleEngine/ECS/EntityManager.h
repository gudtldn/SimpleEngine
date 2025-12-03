#pragma once
#include <atomic>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se::world
{
class SE_CORE_API EntityManager
{
public:
    explicit EntityManager() = default;

    Entity Create();
    void Destroy(Entity entity);

    [[nodiscard]] bool IsValid(Entity entity) const;
    [[nodiscard]] uint32 GetTotalRecordCount() const { return next_id; }

    [[nodiscard]] Array<Entity> GetAliveEntities() const;

private:
    struct EntityRecord
    {
        uint32 generation = 0;
        bool alive = false;
    };

    Array<EntityRecord> entity_records;
    Array<uint32> free_ids;

    std::atomic<uint32> next_id;
};
}
