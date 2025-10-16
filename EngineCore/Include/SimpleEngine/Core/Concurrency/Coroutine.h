#pragma once
#include "Coroutine/Promise.h"
#include "Coroutine/Task.h"


namespace se::core::concurrency
{
/** C++20 Coroutine Task 타입  */
template <typename T>
using Task = coroutine::TaskImpl<T, coroutine::Promise<T>>;
}
