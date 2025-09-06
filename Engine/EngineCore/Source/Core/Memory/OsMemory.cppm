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
     * @param size size 할당할 메모리의 크기 (Byte)
     * @param loc 소스코드 정보 (자동으로 채워집니다)
     * @return 할당된 메모리 블록의 시작 주소를 가리키는 포인터.
     */
    [[nodiscard]] static void* Allocate(size_t size, const std::source_location& loc = std::source_location::current());

    template <typename T>
    [[nodiscard]] static T* Allocate(const std::source_location& loc = std::source_location::current());

    /**
     * 이전에 Allocate로 할당된 메모리를 해제합니다.
     * @param address 해제할 메모리의 시작 주소
     */
    static void Free(void* address);
};

template <typename T>
T* OsMemory::Allocate(const std::source_location& loc)
{
    return static_cast<T*>(Allocate(sizeof(T), loc));
}
}
