#include "Core/Logging/Backends/ConsoleBackend.h"

#include <iostream>
#include <print>

#include "Core/Logging/LogSettings.h"


namespace
{
namespace LogColors
{
constexpr const char* COLOR_DEBUG = "\x1b[36m";   // Cyan
constexpr const char* COLOR_INFO = "\x1b[32m";    // Green
constexpr const char* COLOR_WARNING = "\x1b[33m"; // Yellow
constexpr const char* COLOR_ERROR = "\x1b[31m";   // Red
constexpr const char* COLOR_FATAL = "\x1b[35m";   // Magenta
constexpr const char* COLOR_RESET = "\x1b[0m";    // Reset
}

const char* GetColorForLevel(se::ELogLevel level)
{
    if (!se::core::LogSettings::IsColorEnabled() || !se::core::LogSettings::DetectColorSupport())
    {
        return "";
    }

    switch (level)
    {
    case se::ELogLevel::Debug: return LogColors::COLOR_DEBUG;
    case se::ELogLevel::Info: return LogColors::COLOR_INFO;
    case se::ELogLevel::Warning: return LogColors::COLOR_WARNING;
    case se::ELogLevel::Error: return LogColors::COLOR_ERROR;
    case se::ELogLevel::Fatal: return LogColors::COLOR_FATAL;
    default: return "";
    }
}
}

namespace se::core
{
void ConsoleBackend::WriteLog(const LogEntry& entry)
{
    const char* color = GetColorForLevel(entry.level);
    const char* reset = LogSettings::IsColorEnabled() && LogSettings::DetectColorSupport()
                             ? LogColors::COLOR_RESET
                             : "";

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
