#include "Core/Memory/MemoryResource/OsMemoryResource.h"
#include "Core/Memory/OsMemory.h"


namespace se::core::memory::memory_resource
{
void* OsMemoryResource::do_allocate(usize size, usize align)
{
    return OsMemory::Allocate(size, align);
}

void OsMemoryResource::do_deallocate(
    void* ptr,
    [[maybe_unused]] usize size,
    [[maybe_unused]] usize align
)
{
    OsMemory::Free(ptr);
}

bool OsMemoryResource::do_is_equal(const memory_resource& other) const noexcept
{
    return this == &other;
}
}
