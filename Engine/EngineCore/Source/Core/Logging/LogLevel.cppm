export module SE.Core:Logging.LogLevel;

import SE.Types;


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
