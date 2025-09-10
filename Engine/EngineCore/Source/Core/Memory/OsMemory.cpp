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

    // 이런 느낌으로 할당
    // |                raw_block                | <- 실제 할당된 메모리
    // | offset |   header   |     user_data     | <- std::align으로 정렬 후 사용할 메모리

    // 헤더, 데이터, 정렬 패딩을 모두 담을 공간을 계산
    const size_t total_size_to_alloc = HEADER_SIZE + size + alignment - 1;

    void* raw_block = std::malloc(total_size_to_alloc);
    if (raw_block == nullptr)
    {
        return nullptr;
    }

    // 일단 유저 데이터 위치를 계산
    void* user_ptr = static_cast<uint8*>(raw_block) + HEADER_SIZE;
    size_t space = size + alignment - 1;

    // user_ptr의 위치를 정렬 기준에 맞게 이동
    std::align(alignment, size, user_ptr, space);

    // 이렇게 이동한 user_ptr에서 HEADER_SIZE를 빼서 실제 헤더 위치를 계산
    OsMemoryHeader* header = GetHeaderFromUserPtr(user_ptr);

    // 헤더가 실제 할당된 블럭으로부터 얼만큼 떨어져 있는지 계산
    const size_t offset = reinterpret_cast<uintptr_t>(header) - reinterpret_cast<uintptr_t>(raw_block);

    // 유저가 할당한 메모리 크기와 패딩을 기록하고 헤더 생성자 호출
    std::construct_at(header, size, offset);

    return user_ptr;
}

void* OsMemory::Realloc(void* address, size_t new_size, size_t alignment)
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

    const size_t old_allocated_size = GetAllocatedSize(address);

    // 기존 내용 복사
    const size_t copy_size = math::MathUtility::Min(old_allocated_size, new_size);
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

    // user_ptr로 부터 헤더 위치 계산
    OsMemoryHeader* header = GetHeaderFromUserPtr(address);

    // 헤더에서 패딩을 이용하여 실제 할당된 메모리 블럭 계산
    void* raw_block = reinterpret_cast<uint8*>(header) - header->offset;

    // 헤더 소멸자 호출
    std::destroy_at(header);

    // 메모리 해제
    std::free(raw_block);
}
}
