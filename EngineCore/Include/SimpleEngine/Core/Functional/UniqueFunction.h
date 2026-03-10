#pragma once
#include <functional>
#include <type_traits>
#include <utility>

#include "SimpleEngine/Core/Functional/FunctionDetail.h"


namespace se
{
template <typename Signature>
class UniqueFunction;

/**
 * 이동 전용(move-only) 소유 callable 래퍼
 *
 * @code
 *   auto ptr = std::make_unique<int>(42);
 *   UniqueFunction<void()> fn = [p = std::move(ptr)] { ... }; // OK
 * @endcode
 *
 * @tparam R 반환 타입
 * @tparam Args 인자 타입 팩
 */
template <typename R, typename... Args>
class UniqueFunction<R(Args...)> final : public detail::TFunctionBase<R, Args...>
{
    using Base = detail::TFunctionBase<R, Args...>;

    // 이동 전용 CallableImpl — Clone/CloneTo 없음
    template <typename Fn>
    struct CallableImpl final : detail::ICallable<R, Args...>
    {
        Fn functor;

        template <typename F>
            requires (!std::same_as<std::decay_t<F>, CallableImpl>)
        explicit CallableImpl(F&& in_f)
            : functor(std::forward<F>(in_f))
        {
        }

        virtual R Invoke(Args... args) override
        {
            return std::invoke(functor, std::forward<Args>(args)...);
        }

        virtual detail::ICallable<R, Args...>* MoveTo(void* dest) noexcept override
        {
            return std::construct_at(static_cast<CallableImpl*>(dest), std::move(functor));
        }
    };

public:
    UniqueFunction() noexcept = default;
    UniqueFunction(std::nullptr_t) noexcept {}

    // 복사 금지
    UniqueFunction(const UniqueFunction&) = delete;
    UniqueFunction& operator=(const UniqueFunction&) = delete;

    UniqueFunction(UniqueFunction&& other) noexcept = default;
    UniqueFunction& operator=(UniqueFunction&& other) noexcept = default;

    // 람다 및 기타 callable 객체를 받는 생성자
    // - 자기 자신(UniqueFunction) 제외
    // - nullptr_t 제외 (별도 생성자에서 처리)
    // - 반환 타입을 포함한 호출 가능 여부 검사
    template <typename Fn>
        requires (
            !std::same_as<std::decay_t<Fn>, UniqueFunction>
            && !std::is_null_pointer_v<std::decay_t<Fn>>
            && std::is_invocable_r_v<R, std::decay_t<Fn>&, Args...>
        )
    UniqueFunction(Fn&& in_func)
    {
        this->template Emplace<CallableImpl<std::decay_t<Fn>>>(std::forward<Fn>(in_func));
    }
};
} // namespace se
