#pragma once

#include "SimpleEngine/Core/Functional/FunctionDetail.h"

#include <functional>
#include <type_traits>
#include <utility>


namespace se
{
template <typename Signature>
class Function;

/**
 * 복사·이동 모두 가능한 소유(owning) callable 래퍼
 *
 * @tparam R 반환 타입
 * @tparam Args 인자 타입 팩
 */
template <typename R, typename... Args>
class Function<R(Args...)> final : public detail::TFunctionBase<R, Args...>
{
    using Base = detail::TFunctionBase<R, Args...>;

    struct ICopyableCallable : detail::ICallable<R, Args...>
    {
        virtual ICopyableCallable* Clone() const = 0;             // 힙 복사
        virtual ICopyableCallable* CloneTo(void* dest) const = 0; // SBO 복사
    };

    template <typename Fn>
    struct CopyableCallableImpl final : ICopyableCallable
    {
        static_assert(
            std::is_copy_constructible_v<Fn>,
            "Function<> requires a copy-constructible functor. Use UniqueFunction<> instead."
        );

        Fn functor;

        template <typename F>
            requires (!std::same_as<std::decay_t<F>, CopyableCallableImpl>)
        explicit CopyableCallableImpl(F&& in_f)
            : functor(std::forward<F>(in_f))
        {
        }

        virtual R Invoke(Args... args) override
        {
            return std::invoke(functor, std::forward<Args>(args)...);
        }

        virtual detail::ICallable<R, Args...>* MoveTo(void* dest) noexcept override
        {
            return std::construct_at(static_cast<CopyableCallableImpl*>(dest), std::move(functor));
        }

        virtual ICopyableCallable* Clone() const override
        {
            CopyableCallableImpl* dest = OsMemory::Allocate<CopyableCallableImpl>();
            return std::construct_at(dest, functor);
        }

        virtual ICopyableCallable* CloneTo(void* dest) const override
        {
            return std::construct_at(static_cast<CopyableCallableImpl*>(dest), functor);
        }
    };

    [[nodiscard]] ICopyableCallable* AsCopyable() const noexcept
    {
        // callable_ptr는 항상 CopyableCallableImpl 이므로 안전하게 캐스트 가능
        return static_cast<ICopyableCallable*>(this->callable_ptr);
    }

public:
    Function() noexcept = default;
    Function(std::nullptr_t) noexcept {}

    Function(Function&& other) noexcept = default;
    Function& operator=(Function&& other) noexcept = default;

    Function(const Function& other)
    {
        if (other.callable_ptr)
        {
            if (other.IsOnHeap())
            {
                this->callable_ptr = other.AsCopyable()->Clone();
            }
            else
            {
                this->callable_ptr = other.AsCopyable()->CloneTo(this->sbo_storage);
            }
        }
    }

    Function& operator=(const Function& other)
    {
        if (this != &other)
        {
            this->Reset();
            if (other.callable_ptr)
            {
                if (other.IsOnHeap())
                {
                    this->callable_ptr = other.AsCopyable()->Clone();
                }
                else
                {
                    this->callable_ptr = other.AsCopyable()->CloneTo(this->sbo_storage);
                }
            }
        }
        return *this;
    }

    // 람다 및 기타 callable 객체를 받는 생성자
    // - 자기 자신(Function) 제외
    // - nullptr_t 제외 (별도 생성자에서 처리)
    // - 반환 타입을 포함한 호출 가능 여부 검사
    template <typename Fn>
        requires (
            !std::same_as<std::decay_t<Fn>, Function>
            && !std::is_null_pointer_v<std::decay_t<Fn>>
            && std::is_invocable_r_v<R, std::decay_t<Fn>&, Args...>
        )
    Function(Fn&& in_func)
    {
        this->template Emplace<CopyableCallableImpl<std::decay_t<Fn>>>(std::forward<Fn>(in_func));
    }

    using Base::operator==;

    [[nodiscard]] bool operator==(const Function& other) const
    {
        return this->callable_ptr == other.callable_ptr;
    }
};
} // namespace se
