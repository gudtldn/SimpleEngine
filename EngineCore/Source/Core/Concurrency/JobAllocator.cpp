#include "SimpleEngine/Core/Concurrency/JobAllocator.h"

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Debug.h"

#include "tracy/Tracy.hpp"

#include <atomic>
#include <mutex>
#include <ranges>
#include <utility>


namespace se
{
namespace
{
/** 각 Size Class의 블록 크기 (헤더 포함) */
constexpr FixedArray SIZE_CLASSES = MakeFixedArray<usize>(64, 128, 256, 512);

/** Size Class 개수 */
constexpr usize NUM_SIZE_CLASSES = SIZE_CLASSES.Len();

/** 블록 정렬 크기 - 유저 포인터 앞에 위치 (16바이트 정렬) */
constexpr usize BLOCK_ALIGNMENT = 16;

/**
 * Slab당 블록 수 (배치 할당 단위)
 * @note L1 캐시(32KB~)에 수용 가능한 범위에서 할당 오버헤드를 최소화 할 Slab 배치 단위
 */
constexpr u32 BLOCKS_PER_SLAB = 64;

// TLS 캐시 누적 제한 (초과 시 절반을 Global Pool로 반환하여 메모리 밸런싱)
constexpr u32 MAX_CACHED_BLOCKS = 128;

/** Oversized 블록 표시용 센티널 값 */
constexpr u8 OVERSIZED_CLASS = 0xFF;

/**
 * 블록 헤더
 * 실제 유저 포인터 바로 앞 16바이트에 위치하여 해제 시 Size Class 정보를 복원하는 데 사용됩니다.
 */
struct alignas(BLOCK_ALIGNMENT) BlockHeader
{
    u8 size_class_index; // 0~3: Pool 클래스 Idx, 0xFF: Oversized
};

/**
 * Free List 노드
 * 해제된 블록의 사용자 데이터 영역에 오버레이(Overlay)되어 링크드 리스트를 구성합니다.
 */
struct FreeNode
{
    FreeNode* next;
};

/** 각 스레드별로 유지되는 독립적인 Free List 캐시 */
struct ThreadCache
{
    FixedArray<FreeNode*, NUM_SIZE_CLASSES> free_lists = {};
    FixedArray<u32, NUM_SIZE_CLASSES> counts = {};
};

/** Slow path용 전역 자원 (ABA 방지를 위해 mutex 사용) */
struct alignas(SE_CACHE_LINE) GlobalPool
{
    TracyLockable(std::mutex, lock);
    FreeNode* head = nullptr;
    u32 count = 0;
};

/** 전체 메모리 수거를 위해 할당된 슬랩(Slab)들을 추적하는 Record */
struct SlabRecord
{
    void* memory;
    SlabRecord* next;
};

/** 각 Size Class별로 할당된 GlobalPool 목록 */
FixedArray<GlobalPool, NUM_SIZE_CLASSES> GlobalPools{};

/** 각 Size Class별로 할당된 Slab 리스트의 Head 포인터 목록 */
FixedArray<std::atomic<SlabRecord*>, NUM_SIZE_CLASSES> SlabLists{};

/** TLS 캐시 객체를 반환합니다. */
[[nodiscard]] FORCE_INLINE ThreadCache& GetThreadCache()
{
    thread_local ThreadCache cache;
    return cache;
}

/** 요청된 크기에 적합한 Size Class 인덱스를 탐색합니다. */
[[nodiscard]] FORCE_INLINE usize FindSizeClass(usize in_user_size)
{
    // 유저 크기 + 헤더를 포함한 전체 블록 크기로 Size Class 결정
    const usize total = in_user_size + BLOCK_ALIGNMENT;
    for (const auto [idx, size_class] : SIZE_CLASSES | std::views::enumerate)
    {
        if (total <= size_class)
        {
            return static_cast<usize>(idx);
        }
    }
    return NUM_SIZE_CLASSES; // Oversized
}

/** OS로부터 새 Slab 메모리를 할당받아 블록으로 쪼갠 뒤 해당 스레드의 TLS 캐시에 채워넣습니다. */
void AllocateSlab(usize in_class_index)
{
    SE_ASSERT(in_class_index < NUM_SIZE_CLASSES, "AllocateSlab: invalid class index {}", in_class_index);

    const usize block_size = SIZE_CLASSES[in_class_index];
    const usize slab_size = block_size * BLOCKS_PER_SLAB;

    // Slab 메모리 할당 (캐시 라인 정렬)
    void* memory = OsMemory::Allocate(slab_size, SE_CACHE_LINE);

    // Slab 추적 레코드 등록 (Lock-Free CAS)
    SlabRecord* record = new SlabRecord{};
    record->memory = memory;

    SlabRecord* old_head = SlabLists[in_class_index].load(std::memory_order_relaxed);
    do
    {
        record->next = old_head;
    }
    while (!SlabLists[in_class_index].compare_exchange_weak(
        old_head, record,
        std::memory_order_release, std::memory_order_relaxed
    ));

    // 블록들을 잘라서 TLS Free List에 등록
    u8* base = static_cast<u8*>(memory);
    for (u32 i = 0; i < BLOCKS_PER_SLAB; ++i)
    {
        u8* block_ptr = base + (i * block_size);

        // 헤더 설정
        BlockHeader* header = reinterpret_cast<BlockHeader*>(block_ptr);
        header->size_class_index = static_cast<u8>(in_class_index);

        // 유저 영역을 FreeNode로 사용
        FreeNode* node = reinterpret_cast<FreeNode*>(block_ptr + BLOCK_ALIGNMENT);
        ThreadCache& cache = GetThreadCache();
        node->next = std::exchange(cache.free_lists[in_class_index], node);
        ++cache.counts[in_class_index];
    }
}

/**
 * TLS 캐시가 MAX_CACHED_BLOCKS를 초과했을 때, 절반을 GlobalPool로 반환합니다.
 * @param in_class_index 넘친 Size Class 인덱스
 */
void EvictToGlobal(usize in_class_index)
{
    ThreadCache& cache = GetThreadCache();
    const u32 evict_count = cache.counts[in_class_index] / 2;
    if (evict_count == 0)
    {
        return;
    }

    // TLS 리스트에서 evict_count개 떼어내기
    FreeNode* evict_head = cache.free_lists[in_class_index];
    FreeNode* evict_tail = evict_head;
    for (u32 i = 1; i < evict_count; ++i)
    {
        evict_tail = evict_tail->next;
    }

    // TLS 리스트 앞부분 잘라내기
    cache.free_lists[in_class_index] = evict_tail->next;
    cache.counts[in_class_index] -= evict_count;

    // GlobalPool에 한 번에 연결 (mutex)
    {
        GlobalPool& pool = GlobalPools[in_class_index];

        std::scoped_lock lock{ pool.lock };
        evict_tail->next = std::exchange(pool.head, evict_head);
        pool.count += evict_count;
    }
}

/**
 * GlobalPool에서 블록 뭉치를 TLS 캐시로 가져옵니다.
 * @param in_class_index 요청 Size Class 인덱스
 * @return GlobalPool에 블록이 있어서 성공했으면 true
 */
bool StealFromGlobal(usize in_class_index)
    {
        GlobalPool& pool = GlobalPools[in_class_index];

        FreeNode* stolen_head = nullptr;
        FreeNode* stolen_tail = nullptr;
        u32 stolen_count = 0;
        {
            std::scoped_lock lock{ pool.lock };
            if (pool.head == nullptr)
            {
                return false;
            }

            // 최대 BLOCKS_PER_SLAB / 2개 또는 전부 가져오기
            constexpr u32 MAX_STEAL = BLOCKS_PER_SLAB / 2;
            stolen_head = pool.head;
            FreeNode* cursor = stolen_head;
            stolen_count = 1;

            while (cursor->next != nullptr && stolen_count < MAX_STEAL)
            {
                cursor = cursor->next;
                ++stolen_count;
            }

            pool.head = std::exchange(cursor->next, nullptr);
            pool.count -= stolen_count;

            stolen_tail = cursor;
        }

        // TLS 캐시에 연결
        ThreadCache& cache = GetThreadCache();
        stolen_tail->next = std::exchange(cache.free_lists[in_class_index], stolen_head);
        cache.counts[in_class_index] += stolen_count;

        return true;
    }
} // namespace


void* JobAllocator::Allocate(usize in_size)
{
    const usize class_index = FindSizeClass(in_size);
    if (class_index >= NUM_SIZE_CLASSES)
    {
        // Oversized: OS에서 직접 할당 (헤더 포함)
        void* raw = OsMemory::Allocate(BLOCK_ALIGNMENT + in_size, BLOCK_ALIGNMENT);
        BlockHeader* header = static_cast<BlockHeader*>(raw);
        header->size_class_index = OVERSIZED_CLASS;
        return static_cast<u8*>(raw) + BLOCK_ALIGNMENT;
    }

    ThreadCache& cache = GetThreadCache();
    FreeNode*& list = cache.free_lists[class_index];
    if (list == nullptr)
    {
        // Tier 2 Path: GlobalPool -> Slab 순서로 시도
        if (!StealFromGlobal(class_index))
        {
            AllocateSlab(class_index);
        }
    }

    // TLS Free List에서 Pop
    FreeNode* node = std::exchange(list, list->next);
    --cache.counts[class_index];
    return node;
}

void JobAllocator::Free(void* in_ptr)
{
    if (in_ptr == nullptr)
    {
        return;
    }

    // 헤더 읽기 (유저 포인터 바로 앞 BLOCK_ALIGNMENT 위치)
    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        static_cast<u8*>(in_ptr) - BLOCK_ALIGNMENT
    );

    const u8 class_index = header->size_class_index;
    if (class_index == OVERSIZED_CLASS)
    {
        // Oversized: OS에 반환 (헤더 포인터 기준)
        OsMemory::Free(header);
        return;
    }

    SE_ASSERT(class_index < NUM_SIZE_CLASSES, "Invalid block header: class_index={}", class_index);

    // 호출 스레드의 TLS Free List에 Push
    ThreadCache& cache = GetThreadCache();
    FreeNode* node = static_cast<FreeNode*>(in_ptr);
    node->next = std::exchange(cache.free_lists[class_index], node);
    ++cache.counts[class_index];

    // Eviction: TLS가 넘치면 절반을 GlobalPool로 반환
    if (cache.counts[class_index] > MAX_CACHED_BLOCKS)
    {
        EvictToGlobal(class_index);
    }
}

void JobAllocator::Shutdown()
{
    SE_ASSERT(!JobSystem::IsInitialized(), "JobAllocator::Shutdown must run after JobSystem destruction.");

    // 현재 스레드의 TLS 캐시를 먼저 초기화 (댕글링 포인터 방지)
    ThreadCache& cache = GetThreadCache();
    for (usize i = 0; i < NUM_SIZE_CLASSES; ++i)
    {
        cache.free_lists[i] = nullptr;
        cache.counts[i] = 0;
    }

    // GlobalPool 비우기
    for (GlobalPool& pool : GlobalPools)
    {
        std::scoped_lock lock{ pool.lock };
        pool.head = nullptr;
        pool.count = 0;
    }

    for (std::atomic<SlabRecord*>& slab_list : SlabLists)
    {
        // Slab 한꺼번에 해제
        const SlabRecord* record = slab_list.exchange(nullptr, std::memory_order_acquire);
        while (record != nullptr)
        {
            const SlabRecord* next = record->next;
            OsMemory::Free(record->memory);
            delete record;
            record = next;
        }
    }
}
} // namespace se
