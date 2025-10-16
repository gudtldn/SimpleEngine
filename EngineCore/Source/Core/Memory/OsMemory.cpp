// ReSharper disable CppDFAMemoryLeak
#include "Core/Memory/OsMemory.h"

#include <memory>
#include "tracy/Tracy.hpp"


namespace se::core::memory
{
void* OsMemory::Allocate(size_t size, size_t alignment)
{
    if (size == 0)
    {
        return nullptr;
    }

    // 이런 느낌으로 할당
    // |                  raw_block                  | <- 실제 할당된 메모리
    // |      <offset> size      | ----------------- | <- Header에 저장된 Offset값
    // | padding |     header    |     user_data     | <- std::align으로 정렬 후 사용할 메모리

    // 헤더, 데이터, 정렬 패딩을 모두 담을 공간을 계산
    const size_t total_size_to_alloc = HEADER_SIZE + size + alignment - 1;

    void* raw_block = std::malloc(total_size_to_alloc);
    if (raw_block == nullptr)
    {
        return nullptr;
    }

    // 일단 유저 데이터 위치를 계산
    void* user_ptr = static_cast<uint8*>(raw_block) + HEADER_SIZE;
    size_t space = total_size_to_alloc - HEADER_SIZE; // 정렬 조정을 위해 사용 가능한 공간

    // user_ptr의 위치를 정렬 기준에 맞게 이동
    std::align(alignment, size, user_ptr, space);

    // 이렇게 이동한 user_ptr에서 HEADER_SIZE를 빼서 실제 헤더 위치를 계산
    OsMemoryHeader* header = GetHeaderFromUserPtr(user_ptr);

    // raw_block의 시작점부터 정렬된 user_ptr까지의 오프셋을 계산
    const size_t offset = static_cast<uint8*>(user_ptr) - static_cast<uint8*>(raw_block);

    // 유저가 할당한 메모리 크기와 패딩을 기록
    header->allocated_size = size;
    header->offset = offset;

    // Tracy로 메모리 사용량 추적
    TracyAllocS(user_ptr, size, 16);

    return user_ptr;
}

void* OsMemory::Realloc(void* address, size_t new_size, size_t alignment)
{
    // TODO: Realloc 최적화

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

    const size_t old_allocated_size = GetAllocatedSize(address);

    // 기존 내용 복사
    const size_t copy_size = std::min(old_allocated_size, new_size);
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

    // Tracy에서 메모리 사용량 추적 해제
    TracyFreeS(address, 16);

    // user_ptr로 부터 헤더 위치 계산
    OsMemoryHeader* header = GetHeaderFromUserPtr(address);

    // user_ptr에서 패딩을 이용하여 실제 할당된 메모리 블럭 계산
    void* raw_block = static_cast<uint8*>(address) - header->offset;

    // 메모리 해제
    std::free(raw_block);
}
}
