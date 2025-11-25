#include "Core/HAL/Platform.h"

#if SE_PLATFORM_WINDOWS
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Utility/StringUtils.h"


namespace
{
void SetThreadName(HANDLE handle, const se::String& name)
{
    const int size_needed = MultiByteToWideChar(
        CP_UTF8, 0,
        name.Data(),
        static_cast<int>(name.ByteLen()),
        nullptr, 0
    );

    std::wstring wide_name(size_needed, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        name.Data(),
        static_cast<int>(name.ByteLen()),
        wide_name.data(), size_needed
    );

    (void)SetThreadDescription(handle, wide_name.c_str());
}

se::String GetThreadName(HANDLE handle)
{
    PWSTR data = nullptr;

    // THREAD_QUERY_LIMITED_INFORMATION 권한 필요
    const HRESULT hr = GetThreadDescription(handle, &data);
    if (FAILED(hr) || data == nullptr)
    {
        return {};
    }

    const std::wstring name{ data };
    LocalFree(data);

    return se::utility::ToString(name);
}
}

namespace se::platform
{
void SetThreadName(std::thread& thread, const String& name)
{
    ::SetThreadName(thread.native_handle(), name);
}

void SetCurrentThreadName(const String& name)
{
    ::SetThreadName(GetCurrentThread(), name);
}

String GetThreadName(std::thread& thread)
{
    return ::GetThreadName(thread.native_handle());
}

String GetCurrentThreadName()
{
    return ::GetThreadName(GetCurrentThread());
}

std::filesystem::path GetExecutableDirectory()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path{ path }.parent_path();
}
}
#endif
