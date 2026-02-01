#include "Core/HAL/Platform.h"

#if SE_PLATFORM_LINUX
#include <cstdlib>
#include <string_view>
#include <utility>

#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "Core/Logging/Logging.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"
#include "Utility/Debug.h"


namespace se
{
void Platform::SetThreadName(std::thread& thread, const String& name)
{
    pthread_setname_np(thread.native_handle(), name.CStr());
}

void Platform::SetCurrentThreadName(const String& name)
{
    pthread_setname_np(pthread_self(), name.CStr());
}

String Platform::GetThreadName(std::thread& thread)
{
    char thread_name[16] = { 0 };
    if (pthread_getname_np(thread.native_handle(), thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}

String Platform::GetCurrentThreadName()
{
    char thread_name[16] = { 0 };
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

    const Path absolute_path = FileSystem::Absolute(path);
    if (const auto parent_opt = absolute_path.Parent())
    {
        const String command = String::Format("xdg-open \"{}\"", parent_opt->ToString());
        std::system(command.CStr());
    }
}
}  // namespace se
#endif
