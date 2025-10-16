#include "Core/HAL/Platform.h"

#if SE_PLATFORM_WINDOWS
#include <string>
#include <Windows.h>

#include "Utility/StringUtils.h"


namespace
{
void SetThreadName(HANDLE handle, const se::u8string& name)
{
    const int size_needed = MultiByteToWideChar(
        CP_UTF8, 0,
        reinterpret_cast<const char*>(name.data()),
        static_cast<int>(name.size()),
        nullptr, 0
    );

    se::wstring wide_name(size_needed, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        reinterpret_cast<const char*>(name.data()),
        static_cast<int>(name.size()),
        wide_name.data(), size_needed
    );

    (void)SetThreadDescription(handle, wide_name.c_str());
}

se::u8string GetThreadName(HANDLE handle)
{
    PWSTR data = nullptr;

    // THREAD_QUERY_LIMITED_INFORMATION 권한 필요
    const HRESULT hr = GetThreadDescription(handle, &data);
    if (FAILED(hr) || data == nullptr)
    {
        return {};
    }

    const se::wstring name = data;
    LocalFree(data);

    return se::utility::string::ToU8String(name);
}
}

namespace se::platform
{
void SetThreadName(std::thread& thread, const u8string& name)
{
    ::SetThreadName(thread.native_handle(), name);
}

void SetCurrentThreadName(const u8string& name)
{
    ::SetThreadName(GetCurrentThread(), name);
}

u8string GetThreadName(std::thread& thread)
{
    return ::GetThreadName(thread.native_handle());
}

u8string GetCurrentThreadName()
{
    return ::GetThreadName(GetCurrentThread());
}
}
#endif
