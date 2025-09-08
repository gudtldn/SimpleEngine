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
     * 요청된 크기만큼 메모리를 할당합니다.
     * @param size 할당할 메모리의 크기 (Byte)
     * @return 할당된 메모리의 포인터
     */
    [[nodiscard]] static void* Malloc(size_t size);

    /**
     * 특정 타입 `T`의 객체를 `count`개 만큼 할당하기 위한 템플릿 헬퍼 함수
     * @tparam T 할당할 객체의 타입
     * @param count 할당할 객체의 개수 (기본값: 1)
     * @return 할당된 메모리의 포인터
     */
    template <typename T>
    [[nodiscard]] static T* Malloc(size_t count = 1);

    /**
     * 크기를 변경할 메모리 블록의 포인터
     * @param address 기를 변경할 메모리 블록의 포인터
     * @param new_size 새로운 메모리 블록의 크기 (Byte)
     * @return 크기가 변경된 메모리의 포인터
     */
    [[nodiscard]] static void* Realloc(void* address, size_t new_size);

    /**
     * 이전에 Allocate로 할당된 메모리를 해제합니다.
     * @param address 해제할 메모리의 시작 주소
     */
    static void Free(void* address);
};

template <typename T>
T* OsMemory::Malloc(size_t count)
{
    return static_cast<T*>(Malloc(sizeof(T) * count));
}
}
