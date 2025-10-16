#pragma once
#include "SimpleEngine/Core/Logging/LogData.h"


namespace se::core::logging
{
/**
 * @todo docs
 */
class ILogBackend
{
public:
    virtual ~ILogBackend() = default;

    virtual void WriteLog(const LogEntry& entry) = 0;
    virtual void Flush() = 0;
};
}
