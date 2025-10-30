#pragma once
#include <chrono>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/Platform.h"
#include "SimpleEngine/Core/Logging/LogLevel.h"


/**
 * LogLevel과 std::source_location 정보를 저장하는 구조체
 *
 * @see https://in-neuro.hatenablog.com/entry/2021/12/15/000033
 */
struct LogLevelAndLocation
{
    LogLevelAndLocation(
        ELogLevel in_level,
        const std::source_location& in_location = std::source_location::current(),
        se::String in_thread_name = se::platform::GetCurrentThreadName()
    )
        : level(in_level)
        , location(in_location)
        , thread_name(std::move(in_thread_name))
    {
    }

    ELogLevel level;
    std::source_location location;
    se::String thread_name;
};

/**
 * 로그의 정보를 가지고 있는 구조체
 */
struct LogEntry
{
    // 로그 레벨
    ELogLevel level;

    // ConsoleLog가 호출된 위치 정보
    std::source_location location;

    // Thread 이름
    se::String thread_name;

    // 로그 메시지
    se::String formatted_message;

    // 타임스탬프
    std::chrono::system_clock::time_point timestamp;

    // location에서 파일 이름만 가져옵니다.
    [[nodiscard]] std::string_view GetPrettyFileName() const
    {
        const std::string_view name_view = location.file_name();
        const usize last_slash = name_view.find_last_of("/\\");
        if (last_slash == std::string_view::npos)
        {
            return name_view;
        }
        return name_view.substr(last_slash + 1);
    }

    [[nodiscard]] std::string GetTimestampString() const
    {
        namespace chrono = std::chrono;
        auto zt = chrono::zoned_time{ chrono::current_zone(), timestamp };
        return std::format("{:%Y-%m-%d %H:%M:%S}", zt);
    }

    [[nodiscard]] const char* GetLevelString() const
    {
        switch (level)
        {
        case ELogLevel::Debug: return "Debug";
        case ELogLevel::Info: return "Info";
        case ELogLevel::Warning: return "Warning";
        case ELogLevel::Error: return "Error";
        case ELogLevel::Fatal: return "Fatal";
        default: return "unknown";
        }
    }
};

struct LogOnceKey
{
    se::String file;
    uint32 line;
    uint32 column;


    bool operator==(const LogOnceKey& other) const noexcept
    {
        return file == other.file && line == other.line && column == other.column;
    }

    struct LogOnceKeyHash
    {
        size_t operator()(const LogOnceKey& k) const noexcept
        {
            const size_t h1 = std::hash<se::String>{}(k.file);
            const size_t h2 = std::hash<uint32>{}(k.line);
            const size_t h3 = std::hash<uint32>{}(k.column);
            // 간단한 해시 조합
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
};
