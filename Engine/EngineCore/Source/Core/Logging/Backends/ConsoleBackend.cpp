module SE.Core;
import :Logging.Backends.ConsoleBackend;


namespace
{
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

namespace se::core::logging::backends
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
