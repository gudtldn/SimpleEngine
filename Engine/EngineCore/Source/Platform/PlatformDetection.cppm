module;
#include "Platform/PlatformMacros.h"
export module SimpleEngine.Platform.Detection;


export namespace se::platform_detection
{
#if PLATFORM_WINDOWS
    inline constexpr bool IsWindows = true;
#else
    inline constexpr bool IsWindows = false;
#endif

#if PLATFORM_LINUX
    inline constexpr bool IsLinux = true;
#else
    inline constexpr bool IsLinux = false;
#endif

#if PLATFORM_MACOS
    inline constexpr bool IsMacOS = true;
#else
    inline constexpr bool IsMacOS = false;
#endif
}
