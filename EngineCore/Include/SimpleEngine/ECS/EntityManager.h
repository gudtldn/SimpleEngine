#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/ECS/Entity.h"

#include <atomic>


namespace se
{
/**
 * 엔티티의 생성, 소멸 및 생명주기를 관리하는 클래스
 */
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
} // namespace se
