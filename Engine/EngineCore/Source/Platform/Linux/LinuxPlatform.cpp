module;
#include "Platform/PlatformMacros.h"

#if PLATFORM_LINUX
#include <pthread.h>
#endif

module SE.Platform;

import SE.Types;
import std;


#if PLATFORM_LINUX
namespace se::platform
{
void Platform::SetThreadName(std::thread& thread, const std::u8string& name)
{
    pthread_setname_np(thread.native_handle(), reinterpret_cast<const char*>(name.c_str()));
}

void Platform::SetCurrentThreadName(const std::u8string& name)
{
    pthread_setname_np(pthread_self(), reinterpret_cast<const char*>(name.c_str()));
}
}
#endif
