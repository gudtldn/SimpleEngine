module;
#include "Platform/PlatformMacros.h"
export module SimpleEngine.Platform.Detection;


export namespace PlatformDetection
{
#if PLATFORM_WINDOWS
    static constexpr bool IsWindows = true;
#else
    static constexpr bool IsWindows = false;
#endif

#if PLATFORM_LINUX
    static constexpr bool IsLinux = true;
#else
    static constexpr bool IsLinux = false;
#endif

#if PLATFORM_MACOS
    static constexpr bool IsMacOS = true;
#else
    static constexpr bool IsMacOS = false;
#endif
}
