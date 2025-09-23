module;
#include "Platform/PlatformMacros.h"

#if PLATFORM_LINUX
#include <pthread.h>
#endif

module SE.Platform;

import SE.Types;
import SE.Utility;
import std;


#if PLATFORM_LINUX
namespace se::platform
{
void Platform::SetThreadName(std::thread& thread, const u8string& name)
{
    pthread_setname_np(thread.native_handle(), reinterpret_cast<const char*>(name.c_str()));
}

void Platform::SetCurrentThreadName(const u8string& name)
{
    pthread_setname_np(pthread_self(), reinterpret_cast<const char*>(name.c_str()));
}

u8string Platform::GetThreadName(std::thread& thread)
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string_utils::ToU8String(thread_name);
    }
    return {};
}

u8string Platform::GetCurrentThreadName()
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string_utils::ToU8String(thread_name);
    }
    return {};
}
}
#endif
