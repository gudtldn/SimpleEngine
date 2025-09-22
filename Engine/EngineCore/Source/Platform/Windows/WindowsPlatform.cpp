module;
#include "Platform/PlatformMacros.h"
#include <Windows.h>
module SE.Platform;

import SE.Types;
import std;


#if PLATFORM_WINDOWS
namespace
{
void SetThreadName(HANDLE handle, const std::u8string& name)
{
    const int size_needed = MultiByteToWideChar(
        CP_UTF8, 0,
        reinterpret_cast<const char*>(name.data()),
        static_cast<int>(name.size()),
        nullptr, 0
    );

    std::wstring wide_name(size_needed, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        reinterpret_cast<const char*>(name.data()),
        static_cast<int>(name.size()),
        wide_name.data(), size_needed
    );

    (void)SetThreadDescription(handle, wide_name.c_str());
}
}

namespace se::platform
{
void Platform::SetThreadName(std::thread& thread, const std::u8string& name)
{
    ::SetThreadName(thread.native_handle(), name);
}

void Platform::SetCurrentThreadName(const std::u8string& name)
{
    ::SetThreadName(GetCurrentThread(), name);
}
}
#endif
