module;
#include "Platform/PlatformMacros.h"
export module SimpleEngine.Platform:Detection;


export namespace se::platform::detection
{
#if PLATFORM_WINDOWS
inline constexpr bool IS_WINDOWS = true;
#else
inline constexpr bool IS_WINDOWS = false;
#endif

#if PLATFORM_LINUX
inline constexpr bool IS_LINUX = true;
#else
inline constexpr bool IS_LINUX = false;
#endif

#if PLATFORM_MACOS
inline constexpr bool IS_MAC_OS = true;
#else
inline constexpr bool IS_MAC_OS = false;
#endif
}
