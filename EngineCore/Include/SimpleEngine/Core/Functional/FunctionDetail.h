#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Debug.h"

#include <cstddef>
#include <type_traits>
#include <utility>


namespace se::detail
{
// SBO(Small Buffer Optimization) 설정
// - 정렬: std::max_align_t 에 맞춤
// - 버퍼 크기: 포인터 3개 분량 (24 bytes on x64)
using SBOAlign = std::max_align_t;
constexpr usize SBO_BUFFER_SIZE = sizeof(void*) * 3;

/**
 * Callable 인터페이스 - 이동 전용 최소 인터페이스
 * @tparam R 반환 타입
 * @tparam Args 인자 타입 팩
 */
template <typename R, typename... Args>
struct ICallable
{
    virtual ~ICallable() = default;

    virtual R Invoke(Args... args) = 0;
    virtual ICallable* MoveTo(void* dest) noexcept = 0;
};

/**
 * Function / UniqueFunction의 공통 베이스
 *
 * @tparam R 반환 타입
 * @tparam Args 인자 타입 팩
 */
template <typename R, typename... Args>
class TFunctionBase
{
protected:
    using CallableType = ICallable<R, Args...>;

public:
    TFunctionBase() noexcept = default;

    TFunctionBase(std::nullptr_t) noexcept
    {
    }

    ~TFunctionBase()
    {
        Reset();
    }

    // 복사 연산은 서브클래스 책임
    TFunctionBase(const TFunctionBase&) = delete;
    TFunctionBase& operator=(const TFunctionBase&) = delete;

    TFunctionBase(TFunctionBase&& other) noexcept
    {
        if (other.callable_ptr)
        {
            if (other.IsOnHeap())
            {
                callable_ptr = other.callable_ptr;
            }
            else
            {
                callable_ptr = other.callable_ptr->MoveTo(sbo_storage);
                std::destroy_at(other.callable_ptr); // other.sbo_storage 정리
            }
            other.callable_ptr = nullptr;
        }
    }

    TFunctionBase& operator=(TFunctionBase&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            if (other.callable_ptr)
            {
                if (other.IsOnHeap())
                {
                    callable_ptr = other.callable_ptr;
                }
                else
                {
                    callable_ptr = other.callable_ptr->MoveTo(sbo_storage);
                    std::destroy_at(other.callable_ptr); // other.sbo_storage 정리
                }
                other.callable_ptr = nullptr;
            }
        }
        return *this;
    }

public:
    [[nodiscard]] bool IsValid() const noexcept
    {
        return callable_ptr != nullptr;
    }

    R Invoke(Args... args) const
    {
        SE_ASSERT(callable_ptr, "callable_ptr is nullptr!");
        return callable_ptr->Invoke(std::forward<Args>(args)...);
    }

    R operator()(Args... args) const
    {
        SE_ASSERT(callable_ptr, "callable_ptr is nullptr!");
        return callable_ptr->Invoke(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept
    {
        return !IsValid();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return IsValid();
    }

protected:
    [[nodiscard]] bool IsOnHeap() const noexcept
    {
        return static_cast<const void*>(callable_ptr) != static_cast<const void*>(sbo_storage);
    }

    void Reset() noexcept
    {
        if (callable_ptr)
        {
            std::destroy_at(callable_ptr);

            if (IsOnHeap())
            {
                OsMemory::Free(callable_ptr);
            }

            callable_ptr = nullptr;
        }
    }

    // callable 객체를 SBO 또는 힙에 배치하는 공통 로직
    // ImplType: ICallable 을 구현하는 구체 타입
    template <typename ImplType, typename Fn>
    void Emplace(Fn&& in_func)
    {
        using DecayedFn = std::decay_t<Fn>;

        // SBO 조건: 크기·정렬 적합 + noexcept 이동 가능
        constexpr bool use_sbo =
            sizeof(ImplType) <= SBO_BUFFER_SIZE
            && alignof(ImplType) <= alignof(SBOAlign)
            && std::is_nothrow_move_constructible_v<DecayedFn>;

        if constexpr (use_sbo)
        {
            callable_ptr = std::construct_at(
                reinterpret_cast<ImplType*>(sbo_storage), std::forward<Fn>(in_func));
        }
        else
        {
            ImplType* dest = OsMemory::Allocate<ImplType>();
            callable_ptr = std::construct_at(dest, std::forward<Fn>(in_func));
        }
    }

protected:
    alignas(SBOAlign) uint8 sbo_storage[SBO_BUFFER_SIZE]{};
    CallableType* callable_ptr = nullptr;
};
} // namespace se::detail
