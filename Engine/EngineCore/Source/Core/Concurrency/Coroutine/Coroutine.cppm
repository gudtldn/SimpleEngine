export module SE.Core:Concurrency.Coroutine;
import :Concurrency.Coroutine.Task;
import :Concurrency.Coroutine.Promise;


namespace se::core::concurrency::coroutine
{
/** C++20 Coroutine Task 타입  */
export template <typename T>
using Task = TaskImpl<T, Promise<T>>;
}
