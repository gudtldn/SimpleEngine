#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * 로그 메시지의 심각도(레벨)를 나타내는 Enum
 */
enum class ELogLevel : u8
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
};
} // namespace se
