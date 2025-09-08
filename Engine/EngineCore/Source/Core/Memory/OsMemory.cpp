module;
#include <cstdlib>
module SE.Core;
import :Memory.OsMemory;

import SE.Math;


namespace se::core::memory
{
void* OsMemory::Allocate(size_t size, size_t alignment)
{
    if (size == 0)
    {
        return nullptr;
    }

#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, size);
#endif
}

void* OsMemory::Realloc(void* address, size_t old_size, size_t new_size, size_t alignment)
{
    if (address == nullptr)
    {
        return Allocate(new_size, alignment);
    }

    if (new_size == 0)
    {
        Free(address);
        return nullptr;
    }

    // 새로운 메모리 할당
    void* new_address = Allocate(new_size, alignment);
    if (new_address == nullptr)
    {
        return nullptr;
    }

    // 기존 내용 복사
    const size_t copy_size = math::MathUtility::Min(old_size, new_size);
    std::memcpy(new_address, address, copy_size);

    // 이전 메모리 해제
    Free(address);

    return new_address;
}

void OsMemory::Free(void* address)
{
    if (address == nullptr)
    {
        return;
    }

#if defined(_WIN32)
    _aligned_free(address);
#else
    std::free(address);
#endif
}
}
