#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Common.h"

#include "SDL3/SDL_gpu.h"


// -----------------------------------------------------------------------------
// GPU Debug Markers
// RenderDoc / PIX 등 그래픽스 디버깅 도구에서 커맨드 스트림을 계층적으로 표시합니다.
// D3D12에서는 WinPixEventRuntime.dll이 PATH에 있어야 동작합니다.
// -----------------------------------------------------------------------------

#if SE_ENABLE_DEBUG_TOOLS
    /**
     * 스코프 종료 시 자동으로 SDL_PopGPUDebugGroup을 호출하는 RAII 디버그 그룹
     * @code
     * SE_GPU_DEBUG_GROUP(cmd, "ForwardPass");
     * // ... GPU commands ...
     * // Pop은 스코프 종료 시 자동
     * @endcode
     */
    #define SE_GPU_DEBUG_GROUP(cmd, name) \
        SDL_PushGPUDebugGroup(cmd, name); \
        SE_SCOPE_DEFER { SDL_PopGPUDebugGroup(cmd); }

    /** 커맨드 스트림에 단발성 디버그 라벨을 삽입합니다. */
    #define SE_GPU_DEBUG_LABEL(cmd, text) SDL_InsertGPUDebugLabel(cmd, text)
#else
    #define SE_GPU_DEBUG_GROUP(cmd, name) ((void)0)
    #define SE_GPU_DEBUG_LABEL(cmd, text) ((void)0)
#endif
