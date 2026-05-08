#include "SimpleEngine/Core/HAL/Platform.h"

#if SE_PLATFORM_LINUX
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/Debug.h"

#include <cstdlib>
#include <string_view>
#include <utility>
#include <pthread.h>
#include <string.h>
#include <unistd.h>


namespace se
{
void Platform::SetCurrentThreadName(const String& name)
{
    pthread_setname_np(pthread_self(), name.CStr());
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

void Platform::SetThreadNameImpl(void* native_handle, const String& name)
{
    SE_ASSERT_RELEASE(name.Len() <= 16);

    pthread_t handle = static_cast<pthread_t>(reinterpret_cast<uintptr_t>(native_handle));
    pthread_setname_np(handle, name.CStr());
}

String Platform::GetThreadNameImpl(void* native_handle)
{
    char thread_name[16] = { 0 };
    pthread_t handle = static_cast<pthread_t>(reinterpret_cast<uintptr_t>(native_handle));

    if (pthread_getname_np(handle, thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}
} // namespace se
#endif
