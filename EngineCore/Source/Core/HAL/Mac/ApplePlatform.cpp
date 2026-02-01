#include "Core/HAL/Platform.h"

#if SE_PLATFORM_MACOS
#include <cstdlib>
#include <string_view>
#include <pthread.h>
#include <string.h>

#include "Core/Logging/Logging.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"
#include "Utility/Debug.h"


namespace se
{
void Platform::SetThreadName([[maybe_unused]] std::thread& thread, [[maybe_unused]] const String& name)
{
    // macOS does not support setting the name of another thread by its handle.
    // This function is a no-op on this platform.
}

void Platform::SetCurrentThreadName(const String& name)
{
    // The pthread_setname_np on macOS/BSD sets the name of the calling thread.
    pthread_setname_np(name.CStr());
}

String Platform::GetThreadName(std::thread& thread)
{
    char thread_name[64] = {};
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

String Platform::GetCurrentThreadName()
{
    char thread_name[64] = {};
    if (pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

void Platform::RevealInExplorer(const Path& path)
{
    if (!path.Exists())
    {
        ConsoleLog(ELogLevel::Warning, "Path does not exist: {}", path);
        return;
    }

    // open -R은 파일을 선택하여 Finder에서 보여줌
    const Path absolute_path = FileSystem::Absolute(path);
    const String command = String::Format("open -R \"{}\"", absolute_path);
    std::system(command.CStr());
}
}  // namespace se
#endif
