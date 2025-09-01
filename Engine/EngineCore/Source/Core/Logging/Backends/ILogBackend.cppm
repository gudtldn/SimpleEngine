export module SE.Core:Logging.Backends.ILogBackend;
import :Logging.LogData;


namespace se::core::logging::backends
{
/**
 *
 */
export class ILogBackend
{
public:
    virtual ~ILogBackend() = default;

    virtual void WriteLog(const LogEntry& entry) = 0;
    virtual void Flush() = 0;
};
}
