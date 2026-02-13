// ReSharper disable CppDFAMemoryLeak
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Core/Memory/MemoryStats.h"

#include <cstring>
#include <memory>

#include "tracy/Tracy.hpp"

constexpr usize TRACY_CALLSTACK_DEPTH = 32;

namespace se
{
void* OsMemory::Allocate(usize size, usize alignment)
{
    if (size == 0)
    {
        return nullptr;
    }

    // 이런 느낌으로 할당
    // |                  raw_block                  | <- 실제 할당된 메모리
    // |      <offset> size      | ----------------- | <- Header에 저장된 Offset값
    // | padding |     header    |     user_data     | <- std::align으로 정렬 후 사용할 메모리

    // 헤더, 데이터, 정렬 패딩을 모두 담을 공간 계산
    const usize total_size_to_alloc = HEADER_SIZE + size + alignment - 1;

    void* raw_block = std::malloc(total_size_to_alloc);
    if (raw_block == nullptr)
    {
        return nullptr;
    }

    // 일단 유저 데이터 위치를 계산
    void* user_ptr = static_cast<uint8*>(raw_block) + HEADER_SIZE;
    usize space = total_size_to_alloc - HEADER_SIZE; // 정렬 조정을 위해 사용 가능한 공간

    // user_ptr의 위치를 정렬 기준에 맞게 이동
    std::align(alignment, size, user_ptr, space);

    // 이렇게 이동한 user_ptr에서 HEADER_SIZE를 빼서 실제 헤더 위치를 계산
    OsMemoryHeader* header = GetHeaderFromUserPtr(user_ptr);

    // raw_block의 시작점부터 정렬된 user_ptr까지의 오프셋을 계산
    const usize offset = static_cast<uint8*>(user_ptr) - static_cast<uint8*>(raw_block);

    // 유저가 할당한 메모리 크기와 패딩을 기록
    header->allocated_size = size;
    header->offset = offset;

#if SE_ENABLE_MEMORY_TRACKING
    // 현재 스레드의 활성 태그 가져오기 (TLS)
    const uint32 current_tag = MemoryStats::GetCurrentTag();
    header->tag_id = current_tag;

    MemoryStats::TrackAlloc(current_tag, size);
#endif

    // Tracy로 메모리 사용량 추적
    TracyAllocS(user_ptr, size, TRACY_CALLSTACK_DEPTH);

    return user_ptr;
}

void* OsMemory::Realloc(void* address, usize new_size, usize alignment)
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

    OsMemoryHeader* const memory_header = GetHeaderFromUserPtr(address);

    // new_size가 기존에 할당된 메모리보다 작으면 할당된 크기만 줄이기
    if (new_size <= memory_header->allocated_size)
    {
        [[maybe_unused]] const usize old_size = memory_header->allocated_size;
        memory_header->allocated_size = new_size;

#if SE_ENABLE_MEMORY_TRACKING
        // 줄어든 만큼 통계 차감
        MemoryStats::TrackFree(memory_header->tag_id, old_size - new_size);
#endif

        TracyFreeS(address, TRACY_CALLSTACK_DEPTH);
        TracyAllocS(address, new_size, TRACY_CALLSTACK_DEPTH);

        return address;
    }

    // 크기를 키우는 경우 새로운 메모리 할당
    void* new_address = Allocate(new_size, alignment);
    if (new_address == nullptr)
    {
        return nullptr;
    }

    // 기존 내용 복사
    const usize copy_size = std::min(memory_header->allocated_size, new_size);
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
    TracyFreeS(address, TRACY_CALLSTACK_DEPTH);

    // user_ptr로 부터 헤더 위치 계산
    const OsMemoryHeader* header = GetHeaderFromUserPtr(address);

#if SE_ENABLE_MEMORY_TRACKING
    // 저장해둔 태그 ID로 통계 해제
    MemoryStats::TrackFree(header->tag_id, header->allocated_size);
#endif

    // user_ptr에서 패딩을 이용하여 실제 할당된 메모리 블럭 계산
    void* raw_block = static_cast<uint8*>(address) - header->offset;

    // 메모리 해제
    std::free(raw_block);
}
}  // namespace se
