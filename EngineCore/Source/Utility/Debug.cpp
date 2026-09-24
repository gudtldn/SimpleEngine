#include "SimpleEngine/Utility/Debug.h"

#include <cstdio>
#include <print>

#if SE_PLATFORM_WINDOWS
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent();
#elif SE_PLATFORM_LINUX
#include <fstream>
#include <string>
#endif


namespace se::detail
{
void PrintLogImpl(const std::source_location& loc, std::string_view fmt, std::format_args args) noexcept
{
    const std::string user_message = std::vformat(fmt, args);
    const std::string final_log = std::format(
        "[{}:{}] {}",
        GetPrettyFileName(loc.file_name()),
        loc.line(), user_message
    );

    std::println(stderr, "{}", final_log);
    std::fflush(stderr);
}

void ReportAssertionFailureImpl(const std::source_location& loc, std::string_view expr, std::string_view fmt, std::format_args args) noexcept
{
    if (fmt.empty())
    {
        std::println(stderr, "[{}:{}] Assertion failed: {}", GetPrettyFileName(loc.file_name()), loc.line(), expr);
    }
    else
    {
        const std::string user_msg = std::vformat(fmt, args);
        std::println(stderr, "[{}:{}] Assertion failed: {}\n└─ {}", GetPrettyFileName(loc.file_name()), loc.line(), expr, user_msg);
    }
    std::fflush(stderr);
}

bool IsDebuggerAttached() noexcept
{
#if SE_PLATFORM_WINDOWS
    return IsDebuggerPresent() != 0;
#elif SE_PLATFORM_LINUX
    std::ifstream status_file("/proc/self/status");
    std::string line;
    while (std::getline(status_file, line))
    {
        constexpr std::string_view prefix = "TracerPid:";
        if (line.starts_with(prefix))
        {
            const std::string_view pid = std::string_view(line).substr(prefix.size());
            return pid.find_first_not_of(" \t0") != std::string_view::npos;
        }
    }
    return false;
#else
    return false;
#endif
}
} // namespace se::detail
