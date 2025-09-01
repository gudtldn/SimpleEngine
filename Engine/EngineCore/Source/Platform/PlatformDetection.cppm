module;
#include "Platform/PlatformMacros.h"
export module SE.Platform:Detection;


export namespace se::platform::detection
{
#if PLATFORM_WINDOWS
inline constexpr bool IS_PLATFORM_WINDOWS = true;
#else
inline constexpr bool IS_PLATFORM_WINDOWS = false;
#endif

#if PLATFORM_LINUX
inline constexpr bool IS_PLATFORM_LINUX = true;
#else
inline constexpr bool IS_PLATFORM_LINUX = false;
#endif

#if PLATFORM_MACOS
inline constexpr bool IS_PLATFORM_MAC_OS = true;
#else
inline constexpr bool IS_PLATFORM_MAC_OS = false;
#endif
}
