module SE.Core;
import :Memory.MemoryResource.OsMemoryResource;

import :Memory.OsMemory;


namespace se::core::memory::memory_resource
{
void* OsMemoryResource::do_allocate(
    size_t size,
    [[maybe_unused]] size_t align
)
{
    return OsMemory::Malloc(size);
}

void OsMemoryResource::do_deallocate(
    void* ptr,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] size_t align
)
{
    OsMemory::Free(ptr);
}

bool OsMemoryResource::do_is_equal(const memory_resource& other) const noexcept
{
    return this == &other;
}
}
