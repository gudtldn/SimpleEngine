export module SimpleEngine.Core:Logging.LogLevel;

import SimpleEngine.Types;


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
