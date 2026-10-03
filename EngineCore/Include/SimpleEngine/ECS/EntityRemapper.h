#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se
{
/**
 * Entity를 직렬화할 때 런타임 Entity와 파일에 쓰는 u64 영속 ID를 서로 바꾸는 SerializeContext 서비스
 * 저장할 때는 저장하는 엔티티마다, 로드할 때는 새로 만든 엔티티마다 Add로 짝을 넣은 뒤 직렬화합니다.
 * 영속 ID 0은 null Entity를 뜻합니다.
 */
class SE_CORE_API EntityRemapper
{
public:
    /**
     * entity와 persistent_id를 짝지어 넣습니다.
     * entity가 null이거나 persistent_id가 0이거나, 이미 넣은 entity나 persistent_id면 넣지 않고 false를 돌려줍니다(파일의 중복 ID 등).
     */
    [[nodiscard]] bool Add(Entity entity, u64 persistent_id);

    /**
     * 저장할 때 entity의 영속 ID를 돌려줍니다. 0은 null Entity입니다.
     * 넣지 않은 entity는 저장하지 않는 엔티티를 가리키는 참조라 0을 돌려주고, GetUnresolvedEntityCount()에 셉니다.
     */
    [[nodiscard]] u64 ToPersistentId(Entity entity);

    /**
     * 로드할 때 persistent_id의 Entity를 돌려줍니다. 0은 null Entity입니다.
     * 넣지 않은 ID는 없는 엔티티를 가리키는 참조라 null Entity를 돌려주고, GetUnresolvedIds()에 기록합니다.
     */
    [[nodiscard]] Entity ToEntity(u64 persistent_id);

    /** ToEntity가 찾지 못한 영속 ID를 처음 만난 순서대로 돌려줍니다. 같은 ID는 한 번만 담습니다. */
    [[nodiscard]] ArrayView<const u64> GetUnresolvedIds() const;

    /** ToPersistentId가 Entity를 찾지 못한 횟수를 돌려줍니다. 같은 Entity도 매번 셉니다. */
    [[nodiscard]] usize GetUnresolvedEntityCount() const;

private:
    HashMap<Entity, u64> id_by_entity;
    HashMap<u64, Entity> entity_by_id;
    Array<u64> unresolved_ids;
    usize unresolved_entity_count = 0;
};
} // namespace se
