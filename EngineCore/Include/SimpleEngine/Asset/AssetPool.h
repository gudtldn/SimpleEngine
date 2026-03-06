#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetPayload.h"
#include "SimpleEngine/Asset/HandleTable.h"

#include "tracy/Tracy.hpp"

#include <limits>
#include <mutex>


namespace se::asset
{
/**
 * 로드된 Asset의 Slot(MemoryBlock)을 관리하는 런타임 저장소
 * SlotEntry의 생명주기(생성 및 제거)를 관리합니다.
 */
class SE_CORE_API AssetPool
{
public:
    /**
     * Frame-Epoch 규약을 위한 지연 파괴 엔트리
     * ExchangeAsset의 이전 포인터를 최소 1프레임 이후에 안전하게 해제합니다.
     */
    struct PendingDestroy
    {
        AssetBase* ptr = nullptr;
        void(*destructor)(void*) = nullptr;
        uint64 release_frame = 0;
    };

public:
    AssetPool() = default;
    ~AssetPool();

    // 복사 & 이동 금지
    AssetPool(const AssetPool&) = delete;
    AssetPool& operator=(const AssetPool&) = delete;
    AssetPool(AssetPool&&) = delete;
    AssetPool& operator=(AssetPool&&) = delete;

public:
    /** AssetId로 HandleData를 조회합니다. */
    [[nodiscard]] Optional<HandleData> Find(const AssetId& id) const;

    /** AssetId로 HandleData를 조회하거나 없으면 새로 생성합니다. (Thread-Safe) */
    [[nodiscard]] HandleData FindOrCreate(const AssetId& id, const TypeId& type_id, const AssetPath& asset_path);

    /**
     * 특정 슬롯을 캐시에서 강제로 제거합니다. (Unload)
     * @warning 해당 슬롯을 가리키는 Handle들이 있다면 stale 상태가 될 수 있습니다.
     */
    void Remove(const AssetId& id);

    /**
     * strong_count가 0인(외부 Handle이 없는) 슬롯들을 정리합니다.
     * @return 제거된 슬롯의 개수
     */
    uint32 CollectGarbage();

    /** 현재 캐시된 에셋의 총 개수를 반환합니다. */
    [[nodiscard]] uint32 GetCount() const;

public:
    /**
     * 메모리 예산을 설정합니다.
     * 초과 시 EvictIfOverBudget에서 LRU 기반 해제를 수행합니다.
     */
    void SetMemoryBudget(uint64 budget_bytes);

    /**
     * Eviction Grace Period를 설정합니다.
     * 마지막 로드 후 이 프레임 수만큼은 해제하지 않습니다.
     */
    void SetGraceFrames(uint64 frames);

    /** EvictIfOverBudget에서 프레임당 최대 해제 슬롯 수를 설정합니다. */
    void SetMaxEvictionsPerFrame(uint32 count);

public:
    /**
     * Asset payload를 지연 파괴 큐에 삽입합니다. (Frame-Epoch 보장)
     * @param payload 해제할 에셋 데이터
     * @param current_frame 현재 프레임 번호
     */
    void DeferDestroy(AssetPayload payload, uint64 current_frame);

    /**
     * 지연 파괴 큐에서 해제 시점이 도래한 항목들을 처리합니다.
     * @param current_frame 현재 프레임 번호
     */
    void ProcessPendingDestroy(uint64 current_frame);

    /**
     * 메모리 예산 초과 시 LRU + Scope 우선순위 기반으로 슬롯을 해제합니다.
     * 해제 순서: Transient -> Scene -> Session (Global은 해제하지 않습니다)
     * @param current_frame 현재 프레임 번호
     * @return 해제된 슬롯의 수
     */
    uint32 EvictIfOverBudget(uint64 current_frame);

public:
    /**
     * 특정 scope의 모든 미참조 슬롯을 벌크 해제합니다. (Scene 전환 시 사용)
     * @param layer 해제할 scope 레이어 (Global은 지정할 수 없습니다)
     * @return 해제된 슬롯의 수
     */
    uint32 UnloadScope(EScopeLayer layer);

public:
    /** 현재 전체 에셋 메모리 사용량을 반환합니다. */
    [[nodiscard]] FORCE_INLINE uint64 GetTotalMemoryUsage() const { return table.GetTotalMemoryUsage(); }

    /** 설정된 메모리 예산을 반환합니다. */
    [[nodiscard]] FORCE_INLINE uint64 GetMemoryBudget() const { return memory_budget; }

public:
    /** HandleTable에 직접 접근합니다. */
    [[nodiscard]] FORCE_INLINE HandleTable& GetTable() { return table; }
    [[nodiscard]] FORCE_INLINE const HandleTable& GetTable() const { return table; }

private:
    HandleTable table;

    // === Deferred Destruction ===
    TracyLockable(std::mutex, pending_destroy_mutex);
    Array<PendingDestroy> pending_destroy;

    // === Eviction Configuration === TODO: 여기 EngineConfig.toml로 뺄 수 있을 듯
    uint64 memory_budget = std::numeric_limits<uint64>::max();
    uint64 grace_frames = 2;               // 프레임 유예기간
    uint32 max_evictions_per_frame = 32;   // 메모리 부족시 프레임당 최대 제거 수
    uint32 max_destructions_per_frame = 8; // 일반적인 프레임당 최대 제거 수
};
} // namespace se::asset
