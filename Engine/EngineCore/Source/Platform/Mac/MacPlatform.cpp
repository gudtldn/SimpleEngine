module;
#include "Platform/PlatformMacros.h"

#if PLATFORM_MACOS
#include <pthread.h>
#endif

module SE.Platform;

import SE.Types;
import std;


#if PLATFORM_MACOS
namespace se::platform
{
void Platform::SetThreadName(std::thread& thread, const std::u8string& name)
{
    // macOS does not support setting the name of another thread by its handle.
    // This function is a no-op on this platform.
}

void Platform::SetCurrentThreadName(const std::u8string& name)
{
    // The pthread_setname_np on macOS/BSD sets the name of the calling thread.
    pthread_setname_np(reinterpret_cast<const char*>(name.c_str()));
}
}
#endif
