module;
#include "tracy/Tracy.hpp"
export module SE.Core:Logging;
import :Logging.LogData;
import :Logging.Colors;

export import :Logging.Formatter;
export import :Logging.LogLevel;
export import :Logging.LogSettings;
export import :Logging.LogBackendManager;

export import :Logging.Backends.ILogBackend;
export import :Logging.Backends.FileBackend;
export import :Logging.Backends.ConsoleBackend;

import SE.Types;
import SE.Utility;
import std;


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
    const std::string s = reinterpret_cast<const char*>(log_level.thread_name.data());
    LogEntry entry = {
        .level = log_level.level,
        .location = log_level.location,
        .thread_name = std::move(log_level.thread_name),
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

    using namespace se::core::logging;

    LogBackendManager& manager = LogBackendManager::Get();
    manager.WriteToAllBackends(entry);
    manager.FlushAllBackends();
}

export template <typename... Args>
void ConsoleLogOnce(LogLevelAndLocation log_level, std::u8string_view fmt, const Args&... args)
{
    static se::unordered_set<LogOnceKey, LogOnceKey::LogOnceKeyHash> called_logs;
    static TracyLockable(std::mutex, mtx);

    {
        // 키 생성
        const LogOnceKey key{
            .file = log_level.location.file_name() ? log_level.location.file_name() : "",
            .line = log_level.location.line(),
            .column = log_level.location.column(),
        };

        std::lock_guard lock(mtx);
        if (called_logs.contains(key))
        {
            return; // 이미 호출한 로그면 리턴
        }
        called_logs.insert(key);
    }

    ConsoleLog(log_level, fmt, args...);
}

/** 현재 함수의 Stack Trace를 출력합니다. */
export void PrintStackTrace()
{
    if constexpr (se::utility::IS_DEBUG_BUILD)
    {
        const std::stacktrace stack_trace = std::stacktrace::current();

        ConsoleLog(ELogLevel::Debug, u8"Stack Trace:");
        for (const std::stacktrace_entry& entry : stack_trace | std::views::drop(1) | std::views::reverse)
        {
            ConsoleLog(ELogLevel::Debug, u8"{}", entry);
        }
    }
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
