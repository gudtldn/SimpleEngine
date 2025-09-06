export module SE.Core:ECS.EntityManager;
import :ECS.Entity;

import SE.Types;
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
