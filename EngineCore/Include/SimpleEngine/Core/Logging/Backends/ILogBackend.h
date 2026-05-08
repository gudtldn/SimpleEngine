#pragma once

#include "SimpleEngine/Core/Logging/LogData.h"


namespace se
{
/**
 * 로그 데이터를 실제 출력 대상(파일, 콘솔, 네트워크 등)으로 전달하는 인터페이스
 */
class ILogBackend
{
public:
    virtual ~ILogBackend() = default;

    virtual void WriteLog(const LogEntry& entry) = 0;
    virtual void Flush() = 0;
};
}
