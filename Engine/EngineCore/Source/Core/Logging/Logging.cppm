export module SimpleEngine.Logging;

export import :Formatter;
export import :Colors;

import SimpleEngine.Platform.Types;
import std;


/**
 * 로그 메시지의 심각도(레벨)를 나타내는 Enum
 */
export enum class ELogLevel : uint8
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};

const char8* ToString(ELogLevel e)
{
    switch (e)
    {
    case ELogLevel::Debug: return u8"Debug";
    case ELogLevel::Info: return u8"Info";
    case ELogLevel::Warning: return u8"Warning";
    case ELogLevel::Error: return u8"Error";
    case ELogLevel::Fatal: return u8"Fatal";
    default: return u8"unknown";
    }
}

/**
 * LogLevel과 std::source_location 정보를 저장하는 구조체
 *
 * @see https://in-neuro.hatenablog.com/entry/2021/12/15/000033
 */
struct LogLevelAndLocation
{
    constexpr LogLevelAndLocation(ELogLevel in_level, const std::source_location& in_location = std::source_location::current())
        : level(in_level)
        , location(in_location)
    {
    }

    ELogLevel level;
    std::source_location location;
};

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

/**
 * 로그의 정보를 가지고 있는 구조체
 */
struct LogEntry
{
    // 로그 레벨
    ELogLevel level;

    // ConsoleLog가 호출된 위치 정보
    std::source_location location;

    // 로그 메시지
    std::string formatted_message;

    // 타임스탬프
    std::chrono::system_clock::time_point timestamp;

    // location에서 파일 이름만 가져옵니다.
    std::string_view GetPrettyFileName() const
    {
        const std::string_view name_view = location.file_name();
        const size_t last_slash = name_view.find_last_of("/\\");
        if (last_slash == std::string_view::npos)
        {
            return name_view;
        }
        return name_view.substr(last_slash + 1);
    }

    // TODO: Timestamp 문자열 반환 함수 추가
};

/**
 * Console에 Log를 출력합니다.
 *
 * @param log_level 로그 레벨 (ELogLevel)
 * @param fmt 출력할 메시지의 포맷 문자열
 * @param args 포맷 문자열에 삽입될 가변 인수
 */
export template <typename... Args>
void ConsoleLog(LogLevelAndLocation log_level, std::u8string_view fmt, const Args&... args)
{
    const char8* color = GetColorForLevel(log_level.level);
    const char8* reset = LogSettings::IsColorEnabled() && LogSettings::DetectColorSupport()
                             ? LogColors::COLOR_RESET
                             : u8"";

    LogEntry entry = {
        .level = log_level.level,
        .location = log_level.location,
        .timestamp = std::chrono::system_clock::now(),
    };

    if constexpr (sizeof...(Args) > 0) // 가변 인자가 있을 때만 추가젹인 formatting
    {
#if defined(_DEBUG)
        try
        {
#endif
            entry.formatted_message = std::vformat(std::string(fmt.begin(), fmt.end()), std::make_format_args(args...));
#if defined(_DEBUG)
        }
        catch (const std::format_error& e)
        {
            std::println(
                "[{}:{}] Log Formatting Error: {} (Original format: '{}', Args count: {})",
                entry.GetPrettyFileName(), entry.location.line(), e.what(), fmt, sizeof...(Args)
            );
            std::flush(std::cout);
            return;
        }
#endif
    }
    else
    {
        entry.formatted_message = std::string(fmt.begin(), fmt.end());
    }

    std::println(
        "{}{}\t[{}:{}] {}{}",
        color, ToString(entry.level), entry.GetPrettyFileName(), entry.location.line(), entry.formatted_message, reset
    );
    std::flush(std::cout);
}


#define DECLARE_CONSOLE_LOG(log_level) \
    export template <typename... Args> \
    class ConsoleLog_##log_level \
    { \
    public: \
        ConsoleLog_##log_level(std::u8string_view fmt, const Args&... args, const std::source_location& location = std::source_location::current()) \
        { \
            ConsoleLog(LogLevelAndLocation(ELogLevel::log_level, location), fmt, args...); \
        } \
        ~ConsoleLog_##log_level() = default; \
        ConsoleLog_##log_level(const ConsoleLog_##log_level&) = delete; \
        ConsoleLog_##log_level& operator=(const ConsoleLog_##log_level&) = delete; \
        ConsoleLog_##log_level(ConsoleLog_##log_level&&) = delete; \
        ConsoleLog_##log_level& operator=(ConsoleLog_##log_level&&) = delete; \
    }; \
    template <typename... Args> \
    ConsoleLog_##log_level(std::u8string_view fmt, const Args&... args) -> ConsoleLog_##log_level<Args...>;


/** ConsoleLog에 Debug로 Log를 출력합니다. */
DECLARE_CONSOLE_LOG(Debug)

/** ConsoleLo에 Info로 Log를 출력합니다. */
DECLARE_CONSOLE_LOG(Info)

/** ConsoleLog의 에 Warning로 Log를 출력합니다. */
DECLARE_CONSOLE_LOG(Warning)

/** ConsoleLog에 Error로 Log를 출력합니다. */
DECLARE_CONSOLE_LOG(Error)

/** ConsoleLog에 Fatal로 Log를 출력합니다. */
DECLARE_CONSOLE_LOG(Fatal)
