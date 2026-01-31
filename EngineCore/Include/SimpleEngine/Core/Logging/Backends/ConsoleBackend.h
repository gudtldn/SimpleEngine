#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Logging/Backends/ILogBackend.h"


namespace se
{
/**
 * 표준 출력(stdout/stderr)을 통해 콘솔에 로그를 남기는 Backend 클래스
 */
class SE_CORE_API ConsoleBackend : public ILogBackend
{
public:
    virtual void WriteLog(const LogEntry& entry) override;
    virtual void Flush() override;
};
}
