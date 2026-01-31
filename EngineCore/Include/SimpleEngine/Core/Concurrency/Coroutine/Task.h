#pragma once
#include <coroutine>
#include <exception>
#include <utility>

#include "SimpleEngine/Core/Error/Expected.h"


namespace se::detail
{
/**
 * C++20 Coroutine의 구현부
 * @tparam T 코루틴의 최종 반환 값 타입 (void 가능)
 */
template <typename T, typename PromiseType>
class TaskImpl
{
public:
    using promise_type = PromiseType;

    using HandleType = std::coroutine_handle<PromiseType>;
    HandleType handle;

public:
    explicit TaskImpl(HandleType in_handle = nullptr);
    ~TaskImpl();

    // 코루틴은 복사 불가
    TaskImpl(const TaskImpl&) = delete;
    TaskImpl& operator=(const TaskImpl&) = delete;

    // 이동 생성자
    TaskImpl(TaskImpl&& other) noexcept;
    TaskImpl& operator=(TaskImpl&& other) noexcept;

public:
    // C++20 Awaitable Trait
    /** 'co_await' 시점에 작업이 즉시 완료될 수 있는지(중단 없이) 확인합니다. */
    bool await_ready() noexcept;

    /** await_ready()가 false를 반환했을 때 호출되어 코루틴을 중단(suspend)시킵니다. */
    auto await_suspend(std::coroutine_handle<> awaiting_handle) noexcept;

    /** 코루틴이 재개될 때 'co_await' 표현식의 최종 결과를 반환합니다. */
    T await_resume();
};

template <typename T, typename PromiseType>
TaskImpl<T, PromiseType>::TaskImpl(HandleType in_handle)
    : handle(in_handle)
{
}

template <typename T, typename PromiseType>
TaskImpl<T, PromiseType>::~TaskImpl()
{
    if (handle)
    {
        handle.destroy();
    }
}

template <typename T, typename PromiseType>
TaskImpl<T, PromiseType>::TaskImpl(TaskImpl&& other) noexcept
    : handle(std::exchange(other.handle, nullptr))
{
}

template <typename T, typename PromiseType>
TaskImpl<T, PromiseType>& TaskImpl<T, PromiseType>::operator=(TaskImpl&& other) noexcept
{
    if (this != &other)
    {
        if (handle)
        {
            handle.destroy();
        }
        handle = std::exchange(other.handle, nullptr);
    }
    return *this;
}

template <typename T, typename PromiseType>
bool TaskImpl<T, PromiseType>::await_ready() noexcept
{
    return !handle || handle.done();
}

template <typename T, typename PromiseType>
auto TaskImpl<T, PromiseType>::await_suspend(std::coroutine_handle<> awaiting_handle) noexcept
{
    handle.promise().continuation = awaiting_handle;
    return handle;
}

template <typename T, typename PromiseType>
T TaskImpl<T, PromiseType>::await_resume()
{
    Expected<T, std::exception_ptr>& result = handle.promise().result;
    if (result.HasValue())
    {
        return std::move(result).Value();
    }
    std::rethrow_exception(result.Error());
}
}
