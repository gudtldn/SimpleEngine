module SE.Core;
import :Memory.OsMemory;
import :Memory.MemoryTracker;


namespace se::core::memory
{
void* OsMemory::Allocate(size_t size, const std::source_location& loc)
{
    if (size == 0)
    {
        return nullptr;
    }

    // Alloc Header + 실제 크기
    void* block = std::malloc(HEADER_SIZE + size);
    if (block == nullptr)
    {
        return nullptr;
    }

    // 메모리 헤더에 크기 설정
    MemoryAllocHeader* header = static_cast<MemoryAllocHeader*>(block);
    header->alloc_size = size;

    // 메모리 추적
    MemoryTracker::TrackAllocation(block, size, loc);

    // 헤더를 제외한 실제 사용하는 부분만 반환
    return static_cast<uint8*>(block) + HEADER_SIZE;
}

void OsMemory::Free(void* address)
{
    if (address == nullptr)
    {
        return;
    }

    // 실제 할당된 블럭 계산
    void* block = static_cast<uint8*>(address) - HEADER_SIZE;

    // 메모리 추적 해제
    MemoryTracker::TrackDeallocation(block);

    std::free(block);
}
}
