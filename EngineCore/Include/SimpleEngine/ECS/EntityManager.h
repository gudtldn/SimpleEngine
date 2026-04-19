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
 *
 * 내부적으로 generational arena + intrusive free list를 사용.
 * EntityRecord::next_free가 alive/dead 상태와 free list를 동시에 인코딩한다.
 */
class SE_CORE_API EntityManager
{
public:
    // sentinel 값
    static constexpr uint32 ENTITY_ALIVE = ~uint32{ 0 };         // 0xFFFFFFFF: 살아있는 엔티티
    static constexpr uint32 ENTITY_FREE_LIST_END = ~uint32{ 1 }; // 0xFFFFFFFE: free list의 끝

    explicit EntityManager() = default;

public:
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
        free_list_head = ENTITY_FREE_LIST_END;
        next_id.store(0, std::memory_order_relaxed);
    }

private:
    friend void Serialize(Archive& ar, EntityManager& em)
    {
        // entity records (array of {generation, alive})
        uint64 record_count = em.entity_records.Len();
        ar("records");
        ar.BeginArray(record_count);

        if (ar.IsLoading())
        {
            em.entity_records.Resize(static_cast<usize>(record_count));
        }

        for (uint64 i = 0; i < record_count; ++i)
        {
            ar.BeginObject();
            ar("generation") << em.entity_records[i].generation;

            bool alive = em.entity_records[i].IsAlive();
            ar("alive") << alive;

            if (ar.IsLoading())
            {
                // 임시로 alive 상태만 반영. free list는 루프 후 재구축
                em.entity_records[i].next_free = alive ? ENTITY_ALIVE : ENTITY_FREE_LIST_END;
            }

            ar.EndObject();
        }

        ar.EndArray();

        // next_id
        uint32 next = em.next_id.load(std::memory_order_relaxed);
        ar("next_id") << next;
        if (ar.IsLoading())
        {
            em.next_id.store(next, std::memory_order_relaxed);

            // free list 재구축 (역순 순회 -> 낮은 ID가 먼저 재사용됨)
            em.free_list_head = ENTITY_FREE_LIST_END;
            for (uint32 idx = static_cast<uint32>(record_count); idx-- > 0;)
            {
                if (!em.entity_records[idx].IsAlive())
                {
                    em.entity_records[idx].next_free = em.free_list_head;
                    em.free_list_head = idx;
                }
            }
        }
    }

    struct EntityRecord
    {
        uint32 generation = 0;
        uint32 next_free = ENTITY_FREE_LIST_END; // 기본값 = not alive (Emplace 직후 Create에서 ENTITY_ALIVE로 설정)

        [[nodiscard]] bool IsAlive() const { return next_free == ENTITY_ALIVE; }
    };

    Array<EntityRecord> entity_records;
    uint32 free_list_head = ENTITY_FREE_LIST_END;

    std::atomic<uint32> next_id;
};
} // namespace se
