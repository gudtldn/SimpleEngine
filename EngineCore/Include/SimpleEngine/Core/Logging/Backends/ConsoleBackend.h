#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Logging/Backends/ILogBackend.h"


namespace se::core::logging
{
/**
 * @todo docs
 */
class SE_CORE_API ConsoleBackend : public ILogBackend
{
public:
    virtual void WriteLog(const LogEntry& entry) override;
    virtual void Flush() override;
};
}
