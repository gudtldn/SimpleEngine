export module SimpleEngine.Core:ECS.EntityManager;
import :ECS.Entity;

import SimpleEngine.Types;
import std;


namespace se::core::ecs
{
export class EntityManager
{
public:
    explicit EntityManager(uint32 in_max_entities)
        : entity_records(in_max_entities)
        , max_entities(in_max_entities)
    {
    }

    Entity Create();
    void Destroy(Entity entity);

    [[nodiscard]] bool IsValid(Entity entity) const;
    [[nodiscard]] uint32 GetMaxEntities() const { return max_entities; }

private:
    struct EntityRecord
    {
        uint32 generation = 0;
        bool alive = false;
    };

    std::vector<EntityRecord> entity_records;
    std::vector<uint32> free_ids;

    uint32 max_entities;
    std::atomic<uint32> next_id;
};
}
