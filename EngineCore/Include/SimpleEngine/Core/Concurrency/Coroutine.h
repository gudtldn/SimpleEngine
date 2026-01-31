#pragma once
#include "Coroutine/Promise.h"
#include "Coroutine/Task.h"


namespace se
{
/** C++20 Coroutine Task 타입  */
template <typename T>
using Task = detail::TaskImpl<T, detail::Promise<T>>;
}
