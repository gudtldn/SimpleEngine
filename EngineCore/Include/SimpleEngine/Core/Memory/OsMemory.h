#pragma once
#include <cstddef>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/MemoryConfig.h"


namespace se
{
/**
 * 운영체제의 메모리 할당/해제(malloc, free)를 추상화한 클래스
 */
struct SE_CORE_API OsMemory
{
public:
    /**
     * 요청된 크기와 정렬에 맞춰 메모리를 할당합니다.
     * @param size 할당할 메모리의 크기 (Byte)
     * @param alignment 요청된 정렬 값. 반드시 2의 거듭제곱이어야 합니다.
     * @return 할당된 메모리의 포인터
     */
    [[nodiscard]] static void* Allocate(usize size, usize alignment = alignof(std::max_align_t));

    /**
     * 특정 타입 `T`의 객체를 `count`개 만큼 정렬하여 할당하기 위한 템플릿 헬퍼 함수.
     * @tparam T 할당할 객체의 타입
     * @param count 할당할 객체의 개수 (기본값: 1)
     * @param alignment 요청된 정렬 값. 반드시 2의 거듭제곱이어야 합니다. (기본값 alignof(T))
     * @return 할당된 메모리의 포인터
     */
    template <typename T>
    [[nodiscard]] static T* Allocate(usize count = 1, usize alignment = alignof(T));

    /**
     * 이전에 할당된 정렬된 메모리 블록의 크기를 변경합니다.
     * @param address 크기를 변경할 메모리 블록의 포인터
     * @param new_size 새로운 메모리 블록의 크기 (Byte)
     * @param alignment 원래 할당 시 사용했던 정렬 값
     * @return 크기가 변경된 메모리의 포인터
     */
    [[nodiscard]] static void* Realloc(void* address, usize new_size, usize alignment);

    /**
     * 이전에 Allocate로 할당된 메모리를 해제합니다.
     * @param address 해제할 메모리의 시작 주소
     */
    static void Free(void* address);

public:
    /**
     * 할당한 메모리의 크기를 가져옵니다.
     * @param address OsMemory::Allocate로 할당한 메모리 주소
     * @return 할당한 메모리의 크기
     */
    [[nodiscard]] static usize GetAllocatedSize(const void* address)
    {
        if (address == nullptr)
        {
            return 0;
        }
        return GetHeaderFromUserPtr(address)->allocated_size;
    }

private:
    /**
     * Allocate 정보를 저장하기 위한 헤더
     */
    struct alignas(std::max_align_t) OsMemoryHeader
    {
        usize allocated_size;
        usize offset;

#if SE_ENABLE_MEMORY_TRACKING
        uint32 tag_id;
        uint32 _padding;
#endif
    };

    /** AlignedAllocHeader의 크기 */
    static constexpr usize HEADER_SIZE = sizeof(OsMemoryHeader);

private:
    [[nodiscard]] FORCE_INLINE static OsMemoryHeader* GetHeaderFromUserPtr(void* address)
    {
        return reinterpret_cast<OsMemoryHeader*>(
            static_cast<uint8*>(address) - HEADER_SIZE
        );
    }

    [[nodiscard]] FORCE_INLINE static const OsMemoryHeader* GetHeaderFromUserPtr(const void* address)
    {
        return reinterpret_cast<const OsMemoryHeader*>(
            static_cast<const uint8*>(address) - HEADER_SIZE
        );
    }
};

template <typename T>
T* OsMemory::Allocate(usize count, usize alignment)
{
    return static_cast<T*>(Allocate(sizeof(T) * count, alignment));
}
} // namespace se
