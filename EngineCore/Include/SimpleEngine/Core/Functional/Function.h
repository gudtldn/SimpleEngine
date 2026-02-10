#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
namespace detail
{
using SBOAlign = std::max_align_t;
constexpr usize SBO_BUFFER_SIZE = sizeof(void*) * 3;
}

template <typename Signature>
class Function;

template <typename ReturnType, typename... ParamsType>
class Function<ReturnType(ParamsType...)>
{
private:
    // 내부 저장소 및 호출을 위한 인터페이스
    struct ICallable
    {
        virtual ~ICallable() = default;

        virtual ReturnType Invoke(ParamsType... args) = 0;

        virtual ICallable* Clone() const = 0;               // Heap 복사
        virtual ICallable* CloneTo(void* dest) const = 0;   // SBO 복사
        virtual ICallable* MoveTo(void* dest) noexcept = 0; // 이동
    };

    template <typename Fn>
    struct CallableImpl final : ICallable
    {
        Fn functor;

        template <typename F>
            requires (!std::same_as<std::decay_t<F>, CallableImpl>)
        explicit CallableImpl(F&& f)
            : functor(std::forward<F>(f))
        {
        }

        virtual ReturnType Invoke(ParamsType... args) override
        {
            return std::invoke(functor, std::forward<ParamsType>(args)...);
        }

        virtual ICallable* Clone() const override
        {
            CallableImpl* dest = OsMemory::Allocate<CallableImpl>();
            std::construct_at(dest, functor);
            return dest;
        }

        virtual ICallable* CloneTo(void* dest) const override
        {
            return std::construct_at(static_cast<CallableImpl*>(dest), functor);
        }

        virtual ICallable* MoveTo(void* dest) noexcept override
        {
            return std::construct_at(static_cast<CallableImpl*>(dest), std::move(functor));
        }
    };

private:
    [[nodiscard]] bool IsOnHeap() const noexcept
    {
        return static_cast<const void*>(callable_ptr) != static_cast<const void*>(sbo_storage);
    }

    void Reset() noexcept
    {
        if (callable_ptr)
        {
            // 소멸자 호출
            std::destroy_at(callable_ptr);

            if (IsOnHeap())
            {
                OsMemory::Free(callable_ptr);
            }

            callable_ptr = nullptr;
        }
    }

public:
    Function() noexcept = default;
    Function(std::nullptr_t) noexcept {}

    ~Function()
    {
        Reset();
    }

    Function(const Function& other)
    {
        if (other.callable_ptr)
        {
            if (other.IsOnHeap())
            {
                callable_ptr = other.callable_ptr->Clone();
            }
            else
            {
                callable_ptr = other.callable_ptr->CloneTo(sbo_storage);
            }
        }
    }

    Function(Function&& other) noexcept
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

    Function& operator=(const Function& other)
    {
        if (this != &other)
        {
            Reset();
            if (other.callable_ptr)
            {
                if (other.IsOnHeap())
                {
                    callable_ptr = other.callable_ptr->Clone();
                }
                else
                {
                    callable_ptr = other.callable_ptr->CloneTo(sbo_storage);
                }
            }
        }
        return *this;
    }

    Function& operator=(Function&& other) noexcept
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

    // 람다 및 기타 Callable한 객체를 받는 생성자
    template <typename Fn>
        requires (
            !std::same_as<std::decay_t<Fn>, Function>                              // 자기 자신은 제외
            && !std::is_null_pointer_v<std::decay_t<Fn>>                           // nullptr_t는 별도 생성자에서 처리
            && std::is_invocable_r_v<ReturnType, std::decay_t<Fn>&, ParamsType...> // 호출 가능성 검사 (반환 타입 포함)
        )
    Function(Fn&& in_func)
    {
        using DecayedFn = std::decay_t<Fn>;
        using ImplType = CallableImpl<DecayedFn>;

        // SBO 조건: 객체 크기가 버퍼보다 작고, 이동 생성이 noexcept여야 함
        constexpr bool use_sbo =
            sizeof(ImplType) <= detail::SBO_BUFFER_SIZE
            && alignof(ImplType) <= alignof(detail::SBOAlign)
            && std::is_nothrow_move_constructible_v<DecayedFn>;

        if constexpr (use_sbo)
        {
            callable_ptr = std::construct_at(reinterpret_cast<ImplType*>(sbo_storage), std::forward<Fn>(in_func));
        }
        else
        {
            ImplType* dest = OsMemory::Allocate<ImplType>();
            callable_ptr = std::construct_at(dest, std::forward<Fn>(in_func));
        }
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        return callable_ptr != nullptr;
    }

    ReturnType Invoke(ParamsType... args) const
    {
        SE_ASSERT(callable_ptr, "callable_ptr is nullptr!");
        return callable_ptr->Invoke(std::forward<ParamsType>(args)...);
    }

    ReturnType operator()(ParamsType... args) const
    {
        SE_ASSERT(callable_ptr, "callable_ptr is nullptr!");
        return callable_ptr->Invoke(std::forward<ParamsType>(args)...);
    }

public:
    [[nodiscard]] bool operator==(const Function& other) const
    {
        return callable_ptr == other.callable_ptr;
    }

    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept
    {
        return !IsValid();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return IsValid();
    }

private:
    // SBO 버퍼
    alignas(detail::SBOAlign) uint8 sbo_storage[detail::SBO_BUFFER_SIZE]{};

    // 현재 활성화된 Callable 포인터
    ICallable* callable_ptr = nullptr;
};
}
