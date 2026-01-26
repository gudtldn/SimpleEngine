#include "Core/HAL/Platform.h"

#if SE_PLATFORM_WINDOWS
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include "Core/Logging/Logging.h"
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
}  // namespace

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

void RevealInExplorer(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
    {
        ConsoleLog(ELogLevel::Warning, "Path does not exist: {}", path.string());
        return;
    }

    const std::filesystem::path absolute_path = std::filesystem::absolute(path);
    if (std::filesystem::is_directory(absolute_path))
    {
        ShellExecuteW(nullptr, L"explore", absolute_path.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
    }
    else
    {
        // 파일인 경우: explorer.exe /select,"C:\Path\To\File.txt"
        const std::wstring param = L"/select,\"" + absolute_path.wstring() + L"\"";
        ShellExecuteW(nullptr, L"open", L"explorer.exe", param.c_str(), nullptr, SW_SHOWDEFAULT);
    }
}
}  // namespace se::platform
#endif
