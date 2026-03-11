#pragma once

#include "Coroutine/CoroutinePrimitives.h"
#include "Coroutine/JobTask.h"


namespace se
{
/** 후방 호환성을 위한 별칭. 새 코드에서는 JobTask<T>를 직접 사용하세요. */
template <typename T>
using Task [[deprecated("Use instead JobTask<T>")]] = JobTask<T>;
}
