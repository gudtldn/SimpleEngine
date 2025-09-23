module;
#include "Platform/PlatformMacros.h"

#if PLATFORM_MACOS
#include <pthread.h>
#endif

module SE.Platform;

import SE.Types;
import SE.Utility;
import std;


#if PLATFORM_MACOS
namespace se::platform
{
void Platform::SetThreadName(std::thread& thread, const u8string& name)
{
    // macOS does not support setting the name of another thread by its handle.
    // This function is a no-op on this platform.
}

void Platform::SetCurrentThreadName(const u8string& name)
{
    // The pthread_setname_np on macOS/BSD sets the name of the calling thread.
    pthread_setname_np(reinterpret_cast<const char*>(name.c_str()));
}

u8string Platform::GetThreadName(std::thread& thread)
{
    char thread_name[64] = { 0 };
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string_utils::ToU8String(thread_name);
    }
    return {};
}

u8string Platform::GetCurrentThreadName()
{
    char thread_name[64] = { 0 };
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string_utils::ToU8String(thread_name);
    }
    return {};
}
}
#endif
