#include "Core/Logging/Backends/ConsoleBackend.h"

#include <iostream>
#include <print>

#include "Core/Logging/Formatter.h"
#include "Core/Logging/LogSettings.h"


namespace
{
namespace LogColors
{
constexpr const char8* COLOR_DEBUG = u8"\x1b[36m";   // Cyan
constexpr const char8* COLOR_INFO = u8"\x1b[32m";    // Green
constexpr const char8* COLOR_WARNING = u8"\x1b[33m"; // Yellow
constexpr const char8* COLOR_ERROR = u8"\x1b[31m";   // Red
constexpr const char8* COLOR_FATAL = u8"\x1b[35m";   // Magenta
constexpr const char8* COLOR_RESET = u8"\x1b[0m";    // Reset
}

const char8* GetColorForLevel(ELogLevel level)
{
    if (!LogSettings::IsColorEnabled() || !LogSettings::DetectColorSupport())
    {
        return u8"";
    }

    switch (level)
    {
    case ELogLevel::Debug: return LogColors::COLOR_DEBUG;
    case ELogLevel::Info: return LogColors::COLOR_INFO;
    case ELogLevel::Warning: return LogColors::COLOR_WARNING;
    case ELogLevel::Error: return LogColors::COLOR_ERROR;
    case ELogLevel::Fatal: return LogColors::COLOR_FATAL;
    default: return u8"";
    }
}
}

namespace se::core::logging
{
void ConsoleBackend::WriteLog(const LogEntry& entry)
{
    const char8* color = GetColorForLevel(entry.level);
    const char8* reset = LogSettings::IsColorEnabled() && LogSettings::DetectColorSupport()
                             ? LogColors::COLOR_RESET
                             : u8"";

    std::println(
        "{}{:<7} [{}:{}] {}{}",
        color, entry.GetLevelString(), entry.GetPrettyFileName(), entry.location.line(), entry.formatted_message, reset
    );
}

void ConsoleBackend::Flush()
{
    std::flush(std::cout);
}
}
