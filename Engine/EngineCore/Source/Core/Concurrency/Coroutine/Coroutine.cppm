export module SE.Core:Concurrency.Coroutine;

export import :Concurrency.Coroutine.Awaitables;
export import :Concurrency.Coroutine.Promise;
export import :Concurrency.Coroutine.Task;


export namespace se::core::concurrency::coroutine
{
/** C++20 Coroutine Task 타입  */
template <typename T>
using Task = TaskImpl<T, Promise<T>>;
}
