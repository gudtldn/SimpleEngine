// ReSharper disable CppMemberFunctionMayBeStatic
#pragma once

#include <new>      // For std::bad_alloc, std::align_val_t
#include <limits>   // For std::numeric_limits
#include <utility>  // For std::is_constant_evaluated in C++20+

#include "OsMemory.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Logging/Logging.h" // 예시: 로그 시스템 포함

namespace se::core::memory
{
/**
 * 엔진의 기본 메모리 할당자
 * @tparam T 할당할 객체의 타입
 */
template <typename T>
class DefaultAllocator
{
public:
    using value_type = T;
    using ValueType = T;

    constexpr DefaultAllocator() noexcept = default;

    template <typename U>
    constexpr DefaultAllocator(const DefaultAllocator<U>&) noexcept
    {
    }

public:
    /**
     * n개의 T 객체를 저장할 수 있는 메모리 블록을 할당합니다.
     * @param n 할당할 객체의 수
     * @return 할당된 메모리 블록의 포인터
     * @throws std::bad_alloc 메모리 할당에 실패할 경우
     */
    [[nodiscard]] T* allocate(usize n)
    {
        if (n > std::numeric_limits<usize>::max() / sizeof(T))
        {
            ConsoleLog(ELogLevel::Fatal, u8"Memory allocation size overflow: {} elements of size {}", n, sizeof(T));
            throw std::bad_alloc();
        }

        if (n == 0)
        {
            return nullptr;
        }

        return OsMemory::Allocate<T>(n, alignof(T));
    }

    /**
     * 이전에 allocate를 통해 할당된 메모리 블록을 해제합니다.
     * @param p 해제할 메모리 블록의 포인터 (nullptr가 아니어야 함)
     * @param n 해제할 객체의 수 (allocate 호출 시 사용된 값과 동일해야 함)
     */
    void deallocate(T* p, [[maybe_unused]] usize n)
    {
        if (p == nullptr)
        {
            return;
        }

        OsMemory::Free(p);
    }
};

template <typename T1, typename T2>
constexpr bool operator==(const DefaultAllocator<T1>&, const DefaultAllocator<T2>&) noexcept
{
    return true;
}
}
