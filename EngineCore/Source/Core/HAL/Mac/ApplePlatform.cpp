#include "Core/HAL/Platform.h"

#if SE_PLATFORM_MACOS
#include <string>
#include <pthread.h>

#include "Utility/StringUtils.h"


namespace se::platform
{
void SetThreadName([[maybe_unused]] std::thread& thread, [[maybe_unused]] const u8string& name)
{
    // macOS does not support setting the name of another thread by its handle.
    // This function is a no-op on this platform.
}

void SetCurrentThreadName(const u8string& name)
{
    // The pthread_setname_np on macOS/BSD sets the name of the calling thread.
    pthread_setname_np(reinterpret_cast<const char*>(name.c_str()));
}

u8string GetThreadName(std::thread& thread)
{
    char thread_name[64] = {};
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string::ToU8String(thread_name);
    }
    return {};
}

u8string GetCurrentThreadName()
{
    char thread_name[64] = {};
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string::ToU8String(thread_name);
    }
    return {};
}
}
#endif
