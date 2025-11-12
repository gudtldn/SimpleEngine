#pragma once
#include "Coroutine/Promise.h"
#include "Coroutine/Task.h"


namespace se::concurrency
{
/** C++20 Coroutine Task 타입  */
template <typename T>
using Task = details::TaskImpl<T, details::Promise<T>>;
}
