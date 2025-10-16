#include "Core/Memory/MemoryResource/TrackedMemoryResource.h"
#include "Core/Memory/MemoryTracker.h"


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

    // upstream에 할당을 요청
    void* user_ptr = upstream_resource->allocate(size, align);
    if (user_ptr == nullptr)
    {
        throw std::bad_alloc();
    }

    // 메모리 추적
    MemoryTracker::TrackAllocation(user_ptr);

    return user_ptr;
}

void TrackedMemoryResource::do_deallocate(void* ptr, size_t size, size_t align)
{
    if (ptr == nullptr)
    {
        return;
    }

    // 메모리 추적 해제
    MemoryTracker::TrackDeallocation(ptr);

    // 할당 해제
    upstream_resource->deallocate(ptr, size, align);
}

bool TrackedMemoryResource::do_is_equal(const memory_resource& other) const noexcept
{
    return this == &other;
}
}
