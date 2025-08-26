export module SimpleEngine.Core:ECS.EntityManager;
import :ECS.Entity;

import SimpleEngine.Types;
import std;


namespace se::core::ecs
{
export class EntityManager
{
public:
    explicit EntityManager() = default;

    Entity Create();
    void Destroy(Entity entity);

    [[nodiscard]] bool IsValid(Entity entity) const;
    [[nodiscard]] uint32 GetTotalRecordCount() const { return next_id; }

    [[nodiscard]] std::vector<Entity> GetAliveEntities() const;

private:
    struct EntityRecord
    {
        uint32 generation = 0;
        bool alive = false;
    };

    std::vector<EntityRecord> entity_records;
    std::vector<uint32> free_ids;

    std::atomic<uint32> next_id;
};
}
