#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


// 메모리 추적 활성화 매크로
#if SE_BUILD_DEBUG || TRACY_ENABLE
    #define SE_ENABLE_MEMORY_TRACKING true
#else
    #define SE_ENABLE_MEMORY_TRACKING false
#endif
