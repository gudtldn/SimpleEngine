#pragma once

#include "SimpleEngine/Utility/Debug.h"

#include <functional>
#include <type_traits>
#include <utility>


namespace se
{
template <typename Signature>
class FunctionRef;

/**
 * 비소유(non-owning) 경량 callable 참조 래퍼
 *
 * 포인터 2개(데이터 포인터 + invoke thunk)만 저장하므로 힙 할당이 없습니다.
 * callable을 소유하지 않으므로 참조 대상이 FunctionRef 보다 오래 살아야 합니다.
 *
 * 반드시 함수 파라미터 전달 용도로 사용하세요.
 * @warning 저장·반환 시 댕글링 위험이 있습니다.
 *
 * @code
 *   void ForEach(FunctionRef<void(int)> fn);
 *   ForEach([](int x) { ... }); // OK — 람다 수명이 호출 내에 포함됨
 * @endcode
 *
 * @tparam R 반환 타입
 * @tparam Args 인자 타입 팩
 */
template <typename R, typename... Args>
class FunctionRef<R(Args...)>
{
    using ThunkFn = R(*)(const void*, Args...);

public:
    FunctionRef() = delete;

    // callable 객체를 받는 생성자
    template <typename Fn>
        requires (
            !std::same_as<std::decay_t<Fn>, FunctionRef>
            && !std::is_pointer_v<std::decay_t<Fn>>
            && std::is_invocable_r_v<R, std::decay_t<Fn>&, Args...>
        )
    /* implicit */ FunctionRef(Fn&& in_fn) noexcept
        : data_ptr(std::addressof(in_fn))
        , thunk([](const void* ptr, Args... args) -> R
        {
            using Decayed = std::decay_t<Fn>;
            return std::invoke(
                const_cast<Decayed&>(*static_cast<const Decayed*>(ptr)),
                std::forward<Args>(args)...
            );
        })
    {
    }

    // 함수 포인터를 받는 생성자
    /* implicit */ FunctionRef(R (*fnptr)(Args...)) noexcept
        : data_ptr(reinterpret_cast<const void*>(fnptr))
        , thunk([](const void* ptr, Args... args) -> R
        {
            auto original_fnptr = reinterpret_cast<R(*)(Args...)>(const_cast<void*>(ptr));
            return std::invoke(original_fnptr, std::forward<Args>(args)...);
        })
    {
    }

    [[nodiscard]] bool IsValid() const noexcept
    {
        return thunk != nullptr;
    }

    R Invoke(Args... args) const
    {
        SE_ASSERT(thunk, "FunctionRef is not bound!");
        return thunk(data_ptr, std::forward<Args>(args)...);
    }

    R operator()(Args... args) const
    {
        SE_ASSERT(thunk, "FunctionRef is not bound!");
        return thunk(data_ptr, std::forward<Args>(args)...);
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return IsValid();
    }

private:
    const void* data_ptr = nullptr;
    ThunkFn thunk = nullptr;
};
} // namespace se
