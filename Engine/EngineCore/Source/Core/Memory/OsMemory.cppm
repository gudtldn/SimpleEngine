export module SE.Core:Memory.OsMemory;

import SE.Types;
import std;


export namespace se::core::memory
{
/**
 * 운영체제의 메모리 할당/해제(malloc, free)를 추상화한 클래스
 */
struct OsMemory
{
public:
    /**
     * 요청된 크기와 정렬에 맞춰 메모리를 할당합니다.
     * @param size 할당할 메모리의 크기 (Byte)
     * @param alignment 요청된 정렬 값. 반드시 2의 거듭제곱이어야 합니다.
     * @return 할당된 메모리의 포인터
     */
    [[nodiscard]] static void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

    /**
     * 특정 타입 `T`의 객체를 `count`개 만큼 정렬하여 할당하기 위한 템플릿 헬퍼 함수.
     * T의 정렬(alignof(T))이 자동으로 사용됩니다.
     * @tparam T 할당할 객체의 타입
     * @param count 할당할 객체의 개수 (기본값: 1)
     * @return 할당된 메모리의 포인터
     */
    template <typename T>
    [[nodiscard]] static T* Allocate(size_t count = 1);

    /**
     * 이전에 할당된 정렬된 메모리 블록의 크기를 변경합니다.
     * @param address 크기를 변경할 메모리 블록의 포인터
     * @param old_size 이전 메모리 블록의 크기 (Byte)
     * @param new_size 새로운 메모리 블록의 크기 (Byte)
     * @param alignment 원래 할당 시 사용했던 정렬 값
     * @return 크기가 변경된 메모리의 포인터
     */
    [[nodiscard]] static void* Realloc(void* address, size_t old_size, size_t new_size, size_t alignment);

    /**
     * 이전에 Allocate로 할당된 메모리를 해제합니다.
     * @param address 해제할 메모리의 시작 주소
     */
    static void Free(void* address);
};

template <typename T>
T* OsMemory::Allocate(size_t count)
{
    return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
}
}
