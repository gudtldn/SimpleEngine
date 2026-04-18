#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
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

    /**
     * entity id(슬롯 인덱스)로부터 현재 살아있는 Entity를 복원합니다.
     * 해당 슬롯이 이미 해제되었거나 범위 밖이면 NullOpt을 반환합니다.
     */
    [[nodiscard]] Optional<Entity> TryResolveEntity(uint32 id) const;

    /** 모든 상태를 초기화합니다. */
    void Reset()
    {
        entity_records.Clear();
        free_ids.Clear();
        next_id.store(0, std::memory_order_relaxed);
    }

private:
    friend void Serialize(Archive& ar, EntityManager& em)
    {
        // generation + alive per record
        uint64 record_count = em.entity_records.Len();
        ar("record_count") << record_count;

        if (ar.IsLoading())
        {
            em.entity_records.Resize(static_cast<usize>(record_count));
        }

        for (uint64 i = 0; i < record_count; ++i)
        {
            ar("generation") << em.entity_records[i].generation;
            ar("alive") << em.entity_records[i].alive;
        }

        // free_ids
        ar("free_ids") << em.free_ids;

        // next_id
        uint32 next = em.next_id.load(std::memory_order_relaxed);
        ar("next_id") << next;
        if (ar.IsLoading())
        {
            em.next_id.store(next, std::memory_order_relaxed);
        }
    }

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
