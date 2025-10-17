#include "Core/HAL/Platform.h"

#if SE_PLATFORM_LINUX
#include <string>
#include <utility>

#include <unistd.h>
#include <pthread.h>

#include "Utility/StringUtils.h"


namespace se::platform
{
void SetThreadName(std::thread& thread, const u8string& name)
{
    pthread_setname_np(thread.native_handle(), reinterpret_cast<const char*>(name.c_str()));
}

void SetCurrentThreadName(const u8string& name)
{
    pthread_setname_np(pthread_self(), reinterpret_cast<const char*>(name.c_str()));
}

u8string GetThreadName(std::thread& thread)
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string::ToU8String(thread_name);
    }
    return {};
}

u8string GetCurrentThreadName()
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        return utility::string::ToU8String(thread_name);
    }
    return {};
}

std::filesystem::path GetExecutableDirectory()
{
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::filesystem::path{
        std::string{ result, std::min(count, 0) }
    }.parent_path();
}
}
#endif
