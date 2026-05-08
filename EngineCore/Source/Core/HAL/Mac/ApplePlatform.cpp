#include "SimpleEngine/Core/HAL/Platform.h"

#if SE_PLATFORM_MACOS
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Utility/StringUtils.h"
#include "SimpleEngine/Utility/Debug.h"

#include <cstdlib>
#include <pthread.h>
#include <string.h>
#include <string_view>


namespace se
{
void Platform::SetCurrentThreadName(const String& name)
{
    // The pthread_setname_np on macOS/BSD sets the name of the calling thread.
    pthread_setname_np(name.CStr());
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

void Platform::SetThreadNameImpl([[maybe_unused]] void* native_handle, [[maybe_unused]] const String& name)
{
    // macOS does not support setting the name of another thread by its handle.
    // This function is a no-op on this platform.
}

String Platform::GetThreadNameImpl(void* native_handle)
{
    char thread_name[64] = {};
    pthread_t handle = reinterpret_cast<pthread_t>(native_handle);

    if (pthread_getname_np(handle, thread_name, sizeof(thread_name)) == 0)
    {
        usize len = strnlen(thread_name, sizeof(thread_name));
        return { thread_name, len };
    }
    return {};
}
} // namespace se
#endif
