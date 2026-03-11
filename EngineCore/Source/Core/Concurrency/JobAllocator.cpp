#include "SimpleEngine/Core/Concurrency/JobAllocator.h"

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>


namespace se
{
FixedArray<JobAllocator::GlobalPool, JobAllocator::NUM_SIZE_CLASSES> JobAllocator::global_pools{};
FixedArray<std::atomic<JobAllocator::SlabRecord*>, JobAllocator::NUM_SIZE_CLASSES> JobAllocator::slab_lists{};

void* JobAllocator::Allocate(usize in_size)
{
    const usize class_index = FindSizeClass(in_size);
    if (class_index >= NUM_SIZE_CLASSES)
    {
        // Oversized: OS에서 직접 할당 (헤더 포함)
        void* raw = OsMemory::Allocate(BLOCK_ALIGNMENT + in_size, BLOCK_ALIGNMENT);
        BlockHeader* header = static_cast<BlockHeader*>(raw);
        header->size_class_index = OVERSIZED_CLASS;
        return static_cast<uint8*>(raw) + BLOCK_ALIGNMENT;
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
    FreeNode* node = list;
    list = node->next;
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
        static_cast<uint8*>(in_ptr) - BLOCK_ALIGNMENT
    );

    const uint8 class_index = header->size_class_index;
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
    node->next = cache.free_lists[class_index];
    cache.free_lists[class_index] = node;
    ++cache.counts[class_index];

    // Eviction: TLS가 넘치면 절반을 GlobalPool로 반환
    if (cache.counts[class_index] > MAX_CACHED_BLOCKS)
    {
        EvictToGlobal(class_index);
    }
}

void JobAllocator::Shutdown()
{
    // 현재 스레드의 TLS 캐시를 먼저 초기화 (댕글링 포인터 방지)
    ThreadCache& cache = GetThreadCache();
    for (usize i = 0; i < NUM_SIZE_CLASSES; ++i)
    {
        cache.free_lists[i] = nullptr;
        cache.counts[i] = 0;
    }

    // GlobalPool 비우기
    for (GlobalPool& pool : global_pools)
    {
        std::scoped_lock lock{ pool.lock };
        pool.head = nullptr;
        pool.count = 0;
    }

    for (std::atomic<SlabRecord*>& slab_list : slab_lists)
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

usize JobAllocator::FindSizeClass(usize in_user_size)
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

void JobAllocator::AllocateSlab(usize in_class_index)
{
    SE_ASSERT(in_class_index < NUM_SIZE_CLASSES, "AllocateSlab: invalid class index {}", in_class_index);

    const usize block_size = SIZE_CLASSES[in_class_index];
    const usize slab_size = block_size * BLOCKS_PER_SLAB;

    // Slab 메모리 할당 (캐시 라인 정렬)
    void* memory = OsMemory::Allocate(slab_size, SE_CACHE_LINE);

    // Slab 추적 레코드 등록 (Lock-Free CAS)
    SlabRecord* record = new SlabRecord{};
    record->memory = memory;

    SlabRecord* old_head = slab_lists[in_class_index].load(std::memory_order_relaxed);
    do
    {
        record->next = old_head;
    }
    while (!slab_lists[in_class_index].compare_exchange_weak(
        old_head, record, std::memory_order_release, std::memory_order_relaxed
    ));

    // 블록들을 잘라서 TLS Free List에 등록
    uint8* base = static_cast<uint8*>(memory);
    for (uint32 i = 0; i < BLOCKS_PER_SLAB; ++i)
    {
        uint8* block_ptr = base + (i * block_size);

        // 헤더 설정
        BlockHeader* header = reinterpret_cast<BlockHeader*>(block_ptr);
        header->size_class_index = static_cast<uint8>(in_class_index);

        // 유저 영역을 FreeNode로 사용
        FreeNode* node = reinterpret_cast<FreeNode*>(block_ptr + BLOCK_ALIGNMENT);
        ThreadCache& cache = GetThreadCache();
        node->next = cache.free_lists[in_class_index];
        cache.free_lists[in_class_index] = node;
        ++cache.counts[in_class_index];
    }
}

JobAllocator::ThreadCache& JobAllocator::GetThreadCache()
{
    thread_local ThreadCache cache;
    return cache;
}

void JobAllocator::EvictToGlobal(usize in_class_index)
{
    ThreadCache& cache = GetThreadCache();
    const uint32 evict_count = cache.counts[in_class_index] / 2;
    if (evict_count == 0)
    {
        return;
    }

    // TLS 리스트에서 evict_count개 떼어내기
    FreeNode* evict_head = cache.free_lists[in_class_index];
    FreeNode* evict_tail = evict_head;
    for (uint32 i = 1; i < evict_count; ++i)
    {
        evict_tail = evict_tail->next;
    }

    // TLS 리스트 앞부분 잘라내기
    cache.free_lists[in_class_index] = evict_tail->next;
    cache.counts[in_class_index] -= evict_count;

    // GlobalPool에 한 번에 연결 (mutex)
    {
        GlobalPool& pool = global_pools[in_class_index];

        std::scoped_lock lock{ pool.lock };
        evict_tail->next = pool.head;
        pool.head = evict_head;
        pool.count += evict_count;
    }
}

bool JobAllocator::StealFromGlobal(usize in_class_index)
{
    GlobalPool& pool = global_pools[in_class_index];

    FreeNode* stolen_head = nullptr;
    FreeNode* stolen_tail = nullptr;
    uint32 stolen_count = 0;
    {
        std::scoped_lock lock{ pool.lock };
        if (pool.head == nullptr)
        {
            return false;
        }

        // 최대 BLOCKS_PER_SLAB / 2개 또는 전부 가져오기
        constexpr uint32 max_steal = BLOCKS_PER_SLAB / 2;
        stolen_head = pool.head;
        FreeNode* cursor = stolen_head;
        stolen_count = 1;

        while (cursor->next != nullptr && stolen_count < max_steal)
        {
            cursor = cursor->next;
            ++stolen_count;
        }

        pool.head = cursor->next;
        pool.count -= stolen_count;
        cursor->next = nullptr;

        stolen_tail = cursor;
    }

    // TLS 캐시에 연결
    ThreadCache& cache = GetThreadCache();
    stolen_tail->next = cache.free_lists[in_class_index];
    cache.free_lists[in_class_index] = stolen_head;
    cache.counts[in_class_index] += stolen_count;

    return true;
}
} // namespace se
