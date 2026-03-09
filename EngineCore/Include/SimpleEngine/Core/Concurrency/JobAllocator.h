#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <atomic>

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
/**
 * Lock-Free 고정 크기 블록 풀 할당자
 *
 * Job Payload, 코루틴 프레임 등 빈번한 할당/해제를 위해 설계되었습니다.
 * 4개의 Size Class (64/128/256/512 Byte) 별 Pool을 유지하며, Lock-Free로 할당됩니다.
 *
 * @detail 할당 흐름:
 *         1) 요청 크기에 맞는 Size Class 결정 (유저 크기 + 헤더 8 Byte)
 *         2) TLS Free List에서 Pop (단일 스레드, 무경합)
 *         3) Free List 비었으면 Slab에서 BLOCKS_PER_SLAB개 Batch Allocate
 *         4) 512B 초과 시 OS 직접 할당 (Fallback)
 *
 *         해제 흐름:
 *         1) 블록 헤더에서 Size Class 복원
 *         2) 호출 스레드의 TLS Free List에 Push
 *         3) (다른 스레드에서 해제해도 안전 - 해당 스레드의 Free List에 귀속)
 */
class SE_CORE_API JobAllocator
{
public:
    /** 각 Size Class의 블록 크기 (헤더 포함) */
    static constexpr FixedArray SIZE_CLASSES = MakeFixedArray<usize>(64, 128, 256, 512);

    /** Size Class 개수 */
    static constexpr usize NUM_SIZE_CLASSES = SIZE_CLASSES.Len();

    /** 블록 헤더 크기 - 유저 포인터 앞에 위치 (8바이트 정렬) */
    static constexpr usize BLOCK_HEADER_SIZE = 8;

    /** Slab당 블록 수 (배치 할당 단위) */
    static constexpr uint32 BLOCKS_PER_SLAB = 64;

    /**
     * 각 Size Class에서 유저가 실제로 사용할 수 있는 데이터 크기를 계산합니다.
     * @param in_class_index 0 ~ (NUM_SIZE_CLASSES - 1) 사이의 인덱스
     */
    static constexpr usize UsableSize(usize in_class_index)
    {
        SE_ASSERT(in_class_index < NUM_SIZE_CLASSES);
        return SIZE_CLASSES[in_class_index] - BLOCK_HEADER_SIZE;
    }

public:
    /**
     * 지정된 크기 이상의 메모리 블록을 할당합니다.
     *
     * @param in_size 필요한 유저 데이터 크기 (바이트 단위)
     * @return 할당된 메모리의 시작 포인터 (최소 8바이트 정렬)
     * @note 512B 이하의 요청은 TLS 캐시에서 즉시 할당되며, 초과 시 OS 직접 할당(Fallback)으로 전환됩니다.
     */
    [[nodiscard]] static void* Allocate(usize in_size);

    /**
     * 이전에 Allocate를 통해 할당된 메모리 블록을 해제합니다.
     *
     * @param in_ptr Allocate가 반환한 메모리 포인터. nullptr인 경우 무시됩니다.
     * @note 할당한 스레드와 다른 스레드에서 해제해도 안전하며, 해제 시 호출 스레드의 TLS 캐시로 귀속됩니다.
     */
    static void Free(void* in_ptr);

    /**
     * 할당자에서 관리 중인 모든 메모리(Slab)를 OS에 반환합니다.
     * @warning 엔진 종료 시 단 한 번 호출해야 합니다. 호출 후 Allocate/Free 사용 시 UB 입니다.
     */
    static void Shutdown();

private:
    /** Oversized 블록 표시용 센티널 값 */
    static constexpr uint8 OVERSIZED_CLASS = 0xFF;

    /**
     * 블록 헤더
     * 실제 유저 포인터 바로 앞 8바이트에 위치하여 해제 시 Size Class 정보를 복원하는 데 사용됩니다.
     */
    struct BlockHeader
    {
        uint8 size_class_index; // 0~3: Pool 클래스 Idx, 0xFF: Oversized
    };

    /**
     * Free List 노드
     * 해제된 블록의 사용자 데이터 영역에 오버레이(Overlay)되어 링크드 리스트를 구성합니다.
     */
    struct FreeNode
    {
        FreeNode* next;
    };

    /** 전체 메모리 수거를 위해 할당된 슬랩(Slab)들을 추적하는 Record */
    struct SlabRecord
    {
        void* memory;
        SlabRecord* next;
    };

    /** 각 스레드별로 유지되는 독립적인 Free List 캐시 */
    struct ThreadCache
    {
        FreeNode* free_lists[NUM_SIZE_CLASSES] = {};
    };

    /** 요청된 크기에 적합한 Size Class 인덱스를 탐색합니다. */
    [[nodiscard]] static usize FindSizeClass(usize in_user_size);

    /** OS로부터 새 Slab 메모리를 할당받아 블록으로 쪼갠 뒤 해당 스레드의 TLS 캐시에 채워넣습니다. */
    static void AllocateSlab(usize in_class_index);

    /** 호출 스레드 전용 TLS 캐시 객체를 반환합니다. */
    static ThreadCache& GetThreadCache();

private:
    /** 각 Size Class별로 할당된 Slab 리스트의 Head 포인터 목록 */
    static std::atomic<SlabRecord*> slab_lists[NUM_SIZE_CLASSES];
};
} // namespace se
