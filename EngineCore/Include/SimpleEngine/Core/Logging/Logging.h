#pragma once
#include <chrono>
#include <format>
#include <iostream>
#include <stacktrace>

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Logging/LogBackendManager.h"
#include "SimpleEngine/Core/Logging/LogData.h"

#include "tracy/Tracy.hpp"


/**
 * Console에 Log를 출력합니다.
 *
 * @param log_level 로그 레벨 (ELogLevel)
 * @param fmt 출력할 메시지의 포맷 문자열
 * @param args 포맷 문자열에 삽입될 가변 인수
 */
template <typename... Args>
void ConsoleLog(const LogLevelAndLocation& log_level, std::format_string<Args...> fmt, Args&&... args)
{
    auto& manager = se::core::logging::LogBackendManager::Get();
    manager.WriteToAllBackends({
        .level = log_level.level,
        .location = log_level.location,
        .thread_name = std::move(log_level.thread_name),
        .formatted_message = se::String::Format(fmt, std::forward<Args>(args)...),
        .timestamp = std::chrono::system_clock::now(),
    });
    manager.FlushAllBackends();
}

template <typename... Args>
void ConsoleLogOnce(LogLevelAndLocation log_level, std::format_string<Args...> fmt, Args&&... args)
{
    static se::HashSet<LogOnceKey, LogOnceKey::LogOnceKeyHash> called_logs;
    static TracyLockable(std::mutex, mtx);

    {
        // 키 생성
        const LogOnceKey key{
            .file = log_level.location.file_name() ? log_level.location.file_name() : "",
            .line = log_level.location.line(),
            .column = log_level.location.column(),
        };

        std::scoped_lock lock(mtx);
        if (called_logs.Contains(key))
        {
            return; // 이미 호출한 로그면 리턴
        }
        called_logs.Add(key);
    }

    ConsoleLog(log_level, fmt, std::forward<Args>(args)...);
}

/** 현재 함수의 Stack Trace를 출력합니다. */
SE_CORE_API void PrintStackTrace();


#define DECLARE_CONSOLE_LOG(log_level) \
    template <typename... Args> \
    class ConsoleLog_##log_level \
    { \
    public: \
        ConsoleLog_##log_level(std::format_string<Args...> fmt, Args&&... args, const std::source_location& location = std::source_location::current()) \
        { \
            ConsoleLog(LogLevelAndLocation(ELogLevel::log_level, location), fmt, std::forward<Args>(args)...); \
        } \
        ~ConsoleLog_##log_level() = default; \
        ConsoleLog_##log_level(const ConsoleLog_##log_level&) = delete; \
        ConsoleLog_##log_level& operator=(const ConsoleLog_##log_level&) = delete; \
        ConsoleLog_##log_level(ConsoleLog_##log_level&&) = delete; \
        ConsoleLog_##log_level& operator=(ConsoleLog_##log_level&&) = delete; \
    }; \
    template <typename... Args> \
    ConsoleLog_##log_level(std::format_string<Args...> fmt, Args&&... args) -> ConsoleLog_##log_level<Args...>;


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


#undef DECLARE_CONSOLE_LOG
