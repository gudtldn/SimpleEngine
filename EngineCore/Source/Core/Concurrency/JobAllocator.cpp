#include "SimpleEngine/Core/Concurrency/JobAllocator.h"

#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>


namespace se
{
void* JobAllocator::Allocate(usize in_size)
{
    const usize class_index = FindSizeClass(in_size);
    if (class_index >= NUM_SIZE_CLASSES)
    {
        // Oversized: OS에서 직접 할당 (헤더 포함)
        void* raw = OsMemory::Allocate(BLOCK_HEADER_SIZE + in_size);
        BlockHeader* header = static_cast<BlockHeader*>(raw);
        header->size_class_index = OVERSIZED_CLASS;
        return static_cast<uint8*>(raw) + BLOCK_HEADER_SIZE;
    }

    ThreadCache& cache = GetThreadCache();
    FreeNode*& list = cache.free_lists[class_index];
    if (list == nullptr)
    {
        AllocateSlab(class_index);
    }

    // TLS Free List에서 Pop
    FreeNode* node = list;
    list = node->next;
    return node;
}

void JobAllocator::Free(void* in_ptr)
{
    if (in_ptr == nullptr)
    {
        return;
    }

    // 헤더 읽기 (유저 포인터 바로 앞 BLOCK_HEADER_SIZE 위치)
    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        static_cast<uint8*>(in_ptr) - BLOCK_HEADER_SIZE
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
}

void JobAllocator::Shutdown()
{
    // 현재 스레드의 TLS 캐시를 먼저 초기화 (댕글링 포인터 방지)
    ThreadCache& cache = GetThreadCache();
    for (FreeNode*& free_list : cache.free_lists)
    {
        free_list = nullptr;
    }

    for (std::atomic<SlabRecord*>& slab_list : slab_lists)
    {
        // Slab 한꺼번에 해제
        while (const SlabRecord* record = slab_list.exchange(nullptr, std::memory_order_acquire))
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
    const usize total = in_user_size + BLOCK_HEADER_SIZE;
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
    void* memory = OsMemory::Allocate(slab_size, 64);

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
        FreeNode* node = reinterpret_cast<FreeNode*>(block_ptr + BLOCK_HEADER_SIZE);
        ThreadCache& cache = GetThreadCache();
        node->next = cache.free_lists[in_class_index];
        cache.free_lists[in_class_index] = node;
    }
}

JobAllocator::ThreadCache& JobAllocator::GetThreadCache()
{
    thread_local ThreadCache cache;
    return cache;
}

std::atomic<JobAllocator::SlabRecord*> JobAllocator::slab_lists[NUM_SIZE_CLASSES] = {};
} // namespace se
