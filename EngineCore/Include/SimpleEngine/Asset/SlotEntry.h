#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetPayload.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"

#include <atomic>


namespace se
{
/**
 * Asset의 로딩 상태
 */
enum class ELoadingState : u8
{
    Unloaded, // 데이터 없음 (초기 상태)
    Loading,  // 로딩 중 (Placeholder)
    Loaded,   // 사용 가능
    Failed,   // 로딩 실패
};

/**
 * 에셋 수명 범위 (Eviction 우선순위 결정)
 */
enum class EScopeLayer : u8
{
    Global,    // 엔진 수명 - Evict 불가
    Session,   // 세션 수명 - 게임 종료 시 벌크 해제
    Scene,     // 씬 수명 - 씬 전환 시 벌크 해제 (기본값)
    Transient, // 임시 - 예산 초과 시 최우선 해제 대상
};

/**
 * Asset의 상태와 데이터를 관리하는 제어 블록 클래스
 *
 * - Atomic 필드: lock-free 접근이 가능합니다.
 * - Non-atomic 필드: pool_mutex의 unique_lock 하에서만 변경해야 합니다.
 * - generation: Eviction 시 증가하여 유효하지 않은(stale) 핸들을 감지합니다.
 */
struct SE_CORE_API SlotEntry
{
    enum class ESlotState : u8 { Free, Occupied };

    // === Atomic fields ===
    std::atomic<AssetBase*> asset = nullptr;
    std::atomic<u32> ref_count = 0;
    std::atomic<ELoadingState> state = ELoadingState::Unloaded;

    // === Non-atomic fields ===
    u32 generation = 0;
    TypeId asset_type;
    AssetPath source_path;
    AssetId asset_id;

    using DestructorFn = void(*)(void*);
    DestructorFn destructor = nullptr; // TypeRegistry에서 획득한 리플렉션 소멸자

    // Eviction 메타데이터
    std::atomic<u64> last_access_frame = 0;
    u64 asset_size_bytes = 0;
    EScopeLayer scope = EScopeLayer::Scene;
    ESlotState slot_state = ESlotState::Free;

public:
    /** 기본 생성자입니다. Free 상태의 슬롯을 생성합니다. */
    SlotEntry() = default;

    /** Occupied 상태의 슬롯을 생성합니다. */
    explicit SlotEntry(const AssetId& in_id, const TypeId& in_type, AssetPath in_path);

    // Non-copyable
    SlotEntry(const SlotEntry&) = delete;
    SlotEntry& operator=(const SlotEntry&) = delete;
    SlotEntry(SlotEntry&& other) noexcept;
    SlotEntry& operator=(SlotEntry&& other) noexcept;

    ~SlotEntry() = default;

public:
    /**
     * 현재 슬롯을 새로운 에셋용으로 초기화합니다.
     * @note 세대 번호(generation)는 유지됩니다.
     */
    void Initialize(const AssetId& in_id, const TypeId& in_type, AssetPath in_path);

    /** 슬롯을 완전히 비우고 세대(generation)를 증가시켜 기존 핸들을 무효화합니다. */
    void Clear();

    /**
     * 에셋 Payload(포인터 + 소멸자)를 교체하고, 이전 Payload를 반환합니다.
     * @note 반환된 이전 Payload는 Frame-Epoch 규약에 따라 최소 1프레임 동안 유지해야 합니다.
     */
    [[nodiscard]] AssetPayload ExchangePayload(AssetPayload new_payload);

    /**
     * 상태를 Loading으로 변경합니다. (Unloaded/Failed -> Loading)
     * @return 상태 전환에 성공하면 true를 반환합니다.
     */
    [[nodiscard]] bool BeginLoad();

    /** Loading 상태가 완료될 때까지 대기합니다. */
    void WaitForLoadComplete() const;

    /** 현재 로딩 상태를 반환합니다. */
    [[nodiscard]] FORCE_INLINE ELoadingState GetState() const
    {
        return state.load(std::memory_order_acquire);
    }

    /** 로딩 상태를 설정하고 대기 중인 스레드에 알림(notify)을 보냅니다. */
    FORCE_INLINE void SetState(ELoadingState new_state)
    {
        state.store(new_state, std::memory_order_release);
        state.notify_all();
    }
};
} // namespace se
