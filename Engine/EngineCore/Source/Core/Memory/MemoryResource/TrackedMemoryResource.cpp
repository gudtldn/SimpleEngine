module SE.Core;
import :Memory.MemoryResource.TrackedMemoryResource;

import :Memory.MemoryTracker;


namespace se::core::memory::memory_resource
{
TrackedMemoryResource::TrackedMemoryResource(memory_resource* upstream)
    : upstream_resource(upstream)
{
}

void* TrackedMemoryResource::do_allocate(
    size_t size,
    [[maybe_unused]] size_t align
)
{
    if (size == 0)
    {
        return nullptr;
    }

    // 헤더 + 실제 크기 할당
    void* block = upstream_resource->allocate(HEADER_SIZE + size, alignof(MemoryAllocHeader));
    if (block == nullptr)
    {
        throw std::bad_alloc();
    }

    // 메모리 헤더에 크기 설정
    MemoryAllocHeader* header = static_cast<MemoryAllocHeader*>(block);
    header->alloc_size = size;

    // 메모리 추적
    MemoryTracker::TrackAllocation(block, size, std::source_location::current());

    // 헤더를 제외한 실제 사용하는 부분만 반환
    return static_cast<uint8*>(block) + HEADER_SIZE;
}

void TrackedMemoryResource::do_deallocate(
    void* ptr,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] size_t align
)
{
    if (ptr == nullptr)
    {
        return;
    }

    // 실제 할당된 블럭 계산
    void* block = static_cast<uint8*>(ptr) - HEADER_SIZE;

    // 메모리 추적 해제
    MemoryTracker::TrackDeallocation(block);

    // 할당 해제
    const MemoryAllocHeader* header = static_cast<const MemoryAllocHeader*>(block);
    upstream_resource->deallocate(block, HEADER_SIZE + header->alloc_size, alignof(MemoryAllocHeader));
}

bool TrackedMemoryResource::do_is_equal(const memory_resource& other) const noexcept
{
    return this == &other;
}
}
