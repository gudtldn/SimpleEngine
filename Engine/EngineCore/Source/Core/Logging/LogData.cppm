export module SimpleEngine.Core:Logging.LogData;
import :Logging.LogLevel;

import std;


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

    std::string GetTimestampString() const
    {
        namespace chrono = std::chrono;
        auto zt = chrono::zoned_time{ chrono::current_zone(), timestamp };
        return std::format("{:%Y-%m-%d %H:%M:%S}", zt);
    }
};

struct LogOnceKey
{
    std::string file;
    uint32 line;
    uint32 column;


    bool operator==(const LogOnceKey& other) const noexcept
    {
        return file == other.file && line == other.line && column == other.column;
    }

    struct LogOnceKeyHash
    {
        std::size_t operator()(const LogOnceKey& k) const noexcept
        {
            const std::size_t h1 = std::hash<std::string>{}(k.file);
            const std::size_t h2 = std::hash<uint32>{}(k.line);
            const std::size_t h3 = std::hash<uint32>{}(k.column);
            // 간단한 해시 조합
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
};
