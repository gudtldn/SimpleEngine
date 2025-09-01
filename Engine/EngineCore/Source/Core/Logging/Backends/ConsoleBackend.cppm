export module SE.Core:Logging.Backends.ConsoleBackend;
import :Logging.Backends.ILogBackend;


namespace se::core::logging::backends
{
export class ConsoleBackend : public ILogBackend
{
public:
    virtual void WriteLog(const LogEntry& entry) override;
    virtual void Flush() override;
};
}
