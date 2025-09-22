export module SE.Core:Concurrency.Coroutine.Awaitables;

import SE.Types;
import std;


export namespace se::core::concurrency::coroutine
{
/**
 * Task<T>를 WorkerThread에서 실행합니다.
 */
struct SwitchToWorkerThread
{
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) const;
    void await_resume() const noexcept;
};

/**
 * Task<T>를 MainThread에서 실행합니다.
 */
struct SwitchToMainThread
{
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) const;
    void await_resume() const noexcept;
};
}
