#pragma once
#include <atomic>

#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/World/Entity.h"


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

    [[nodiscard]] vector<Entity> GetAliveEntities() const;

private:
    struct EntityRecord
    {
        uint32 generation = 0;
        bool alive = false;
    };

    vector<EntityRecord> entity_records;
    vector<uint32> free_ids;

    std::atomic<uint32> next_id;
};
}
