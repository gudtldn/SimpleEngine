#include "SimpleEngine/Core/HAL/Platform.h"

#if SE_PLATFORM_WINDOWS

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Utility/StringUtils.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>

#include <string>
#include <format>


namespace
{
std::wstring ConvertToWString(const se::String& str)
{
    const int size_needed = MultiByteToWideChar(
        CP_UTF8, 0,
        str.Data(),
        static_cast<int>(str.ByteLen()),
        nullptr, 0
    );

    std::wstring wide_name(size_needed, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        str.Data(),
        static_cast<int>(str.ByteLen()),
        wide_name.data(), size_needed
    );

    return wide_name;
}

void SetThreadName(HANDLE handle, const se::String& name)
{
    const std::wstring wide_name = ConvertToWString(name);
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

    return se::StringUtils::ToString(name);
}
}  // namespace

namespace se
{
void Platform::SetThreadName(std::thread& thread, const String& name)
{
    ::SetThreadName(reinterpret_cast<HANDLE>(thread.native_handle()), name);
}

void Platform::SetCurrentThreadName(const String& name)
{
    ::SetThreadName(GetCurrentThread(), name);
}

String Platform::GetThreadName(std::thread& thread)
{
    return ::GetThreadName(reinterpret_cast<HANDLE>(thread.native_handle()));
}

String Platform::GetCurrentThreadName()
{
    return ::GetThreadName(GetCurrentThread());
}

void Platform::RevealInExplorer(const Path& path)
{
    if (!path.Exists())
    {
        ConsoleLog(ELogLevel::Warning, "Path does not exist: {}", path);
        return;
    }

    const Path absolute_path = FileSystem::Absolute(path);
    std::wstring wstr_path = ConvertToWString(absolute_path.ToString());
    std::ranges::replace(wstr_path, L'/', L'\\');

    if (absolute_path.IsDirectory())
    {
        ShellExecuteW(nullptr, L"explore", wstr_path.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
    }
    else
    {
        // 파일인 경우: explorer.exe /select,"C:\Path\To\File.txt"
        const std::wstring param = std::format(L"/select,\"{}\"", wstr_path);
        ShellExecuteW(nullptr, L"open", L"explorer.exe", param.c_str(), nullptr, SW_SHOWDEFAULT);
    }
}
} // namespace se
#endif
