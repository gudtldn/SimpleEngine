#include "SimpleEngine/Utility/Debug.h"

#include <cstdio>
#include <print>


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
} // namespace se::detail
