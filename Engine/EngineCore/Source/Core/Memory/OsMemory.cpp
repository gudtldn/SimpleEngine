module SE.Core;
import :Memory.OsMemory;


namespace se::core::memory
{
void* OsMemory::Malloc(size_t size)
{
    if (size == 0)
    {
        return nullptr;
    }

    return std::malloc(size);
}

void* OsMemory::Calloc(size_t count, size_t size)
{
    return std::calloc(count, size);
}

void* OsMemory::Realloc(void* address, size_t new_size)
{
    return std::realloc(address, new_size);
}

void OsMemory::Free(void* address)
{
    if (address == nullptr)
    {
        return;
    }

    std::free(address);
}
}
