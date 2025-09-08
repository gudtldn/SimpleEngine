module SE.Core;
import :Memory.MemoryResource.TrackedMemoryResource;

import :Memory.MemoryTracker;


namespace se::core::memory::memory_resource
{
TrackedMemoryResource::TrackedMemoryResource(memory_resource* upstream)
    : upstream_resource(upstream)
{
}

void* TrackedMemoryResource::do_allocate(size_t size, size_t align)
{
    if (size == 0)
    {
        return nullptr;
    }

    // 이런 느낌으로 할당
    // |                raw_block                | <- 실제 할당된 메모리
    // | padding |   header   |    user_data     | <- std::align으로 정렬 후 사용할 메모리

    // 헤더, 데이터, 정렬 패딩을 모두 담을 공간을 계산
    const size_t total_size_to_alloc = HEADER_SIZE + size + align - 1;

    // upstream에 할당을 요청
    void* raw_block = upstream_resource->allocate(total_size_to_alloc, alignof(MemoryAllocHeader));
    if (raw_block == nullptr)
    {
        throw std::bad_alloc();
    }

    // 일단 유저 데이터 위치를 계산
    void* user_ptr = static_cast<uint8*>(raw_block) + HEADER_SIZE;
    size_t space = size + align - 1;

    // user_ptr의 위치를 정렬 기준에 맞게 이동
    std::align(align, size, user_ptr, space);

    // 이렇게 이동한 user_ptr에서 HEADER_SIZE를 빼서 실제 헤더 위치를 계산
    MemoryAllocHeader* header = std::launder(
        reinterpret_cast<MemoryAllocHeader*>(static_cast<uint8*>(user_ptr) - HEADER_SIZE)
    );

    // 유저가 할당한 메모리 크기를 기록
    header->alloc_size = size;

    // 헤더가 실제 할당된 블럭으로부터 얼만큼 떨어져 있는지 계산
    header->padding = static_cast<uint8>(
        reinterpret_cast<uintptr_t>(header) - reinterpret_cast<uintptr_t>(raw_block)
    );

    // 메모리 추적
    MemoryTracker::TrackAllocation(header, size, std::source_location::current());

    return user_ptr;
}

void TrackedMemoryResource::do_deallocate(
    void* ptr,
    [[maybe_unused]] size_t size,
    size_t align
)
{
    if (ptr == nullptr)
    {
        return;
    }

    // user_ptr로 부터 헤더 위치 계산
    const MemoryAllocHeader* header = reinterpret_cast<const MemoryAllocHeader*>(
        static_cast<const uint8*>(ptr) - HEADER_SIZE
    );

    // 메모리 추적 해제
    MemoryTracker::TrackDeallocation(header);

    // 헤더에서 패딩을 이용하여 실제 할당된 메모리 블럭 계산
    const void* raw_block = reinterpret_cast<const uint8*>(header) - header->padding;

    // 할당 해제
    const size_t total_size_to_alloc = HEADER_SIZE + header->alloc_size + align - 1;
    upstream_resource->deallocate(
        const_cast<void*>(raw_block),
        total_size_to_alloc,
        alignof(MemoryAllocHeader)
    );
}

bool TrackedMemoryResource::do_is_equal(const memory_resource& other) const noexcept
{
    return this == &other;
}
}
