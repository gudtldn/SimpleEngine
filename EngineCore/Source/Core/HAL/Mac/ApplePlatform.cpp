#include "Core/HAL/Platform.h"

#if SE_PLATFORM_MACOS
#include <string_view>
#include <pthread.h>
#include <string.h>

#include "Utility/StringUtils.h"


namespace se::platform
{
void SetThreadName([[maybe_unused]] std::thread& thread, [[maybe_unused]] const String& name)
{
    // macOS does not support setting the name of another thread by its handle.
    // This function is a no-op on this platform.
}

void SetCurrentThreadName(const String& name)
{
    // The pthread_setname_np on macOS/BSD sets the name of the calling thread.
    pthread_setname_np(name.CStr());
}

String GetThreadName(std::thread& thread)
{
    char thread_name[64] = {};
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

String GetCurrentThreadName()
{
    char thread_name[64] = {};
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

std::filesystem::path GetExecutableDirectory()
{
    char path_buffer[1024];
    uint32 size = sizeof(path_buffer);
    if (_NSGetExecutablePath(path_buffer, &size) == 0)
    {
        return std::filesystem::path{
            std::string_view(path_buffer)
        }.parent_path();
    }
    return {};
}
}
#endif
