module;
#include "Platform/PlatformMacros.h"
export module SE.Platform:Detection;


export namespace se::platform::detection
{
#if PLATFORM_WINDOWS
constexpr bool IS_PLATFORM_WINDOWS = true;
#else
constexpr bool IS_PLATFORM_WINDOWS = false;
#endif

#if PLATFORM_LINUX
constexpr bool IS_PLATFORM_LINUX = true;
#else
constexpr bool IS_PLATFORM_LINUX = false;
#endif

#if PLATFORM_MACOS
constexpr bool IS_PLATFORM_MAC_OS = true;
#else
constexpr bool IS_PLATFORM_MAC_OS = false;
#endif
}
