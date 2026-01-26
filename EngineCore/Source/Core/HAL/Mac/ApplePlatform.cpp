#include "Core/HAL/Platform.h"

#if SE_PLATFORM_MACOS
#include <cstdlib>
#include <string_view>
#include <pthread.h>
#include <string.h>

#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"
#include "Utility/Debug.h"


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

void RevealInExplorer(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
    {
        ConsoleLog(ELogLevel::Warning, "Path does not exist: {}", path.string());
        return;
    }

    const std::filesystem::path absolute_path = std::filesystem::absolute(path);

    std::string command = "open -R \"" + absolute_path.string() + "\"";
    std::system(command.c_str());
}
}  // namespace se::platform
#endif
