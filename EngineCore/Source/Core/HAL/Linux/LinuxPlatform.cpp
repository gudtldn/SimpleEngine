#include "Core/HAL/Platform.h"

#if SE_PLATFORM_LINUX
#include <string_view>
#include <utility>

#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "Utility/StringUtils.h"


namespace se::platform
{
void SetThreadName(std::thread& thread, const String& name)
{
    pthread_setname_np(thread.native_handle(), name.CStr());
}

void SetCurrentThreadName(const String& name)
{
    pthread_setname_np(pthread_self(), name.CStr());
}

String GetThreadName(std::thread& thread)
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

String GetCurrentThreadName()
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

std::filesystem::path GetExecutableDirectory()
{
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count > 0 && count < PATH_MAX)
    {
        return std::filesystem::path{
            std::string_view(result, static_cast<size_t>(count))
        }.parent_path();
    }
    return {};
}
}
#endif
