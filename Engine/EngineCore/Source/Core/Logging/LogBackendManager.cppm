export module SE.Core:Logging.LogBackendManager;
import :Logging.Backends.ILogBackend;
import :Logging.LogData;

import std;


namespace se::core::logging
{
export class LogBackendManager
{
private:
    LogBackendManager() = default;

public:
    ~LogBackendManager() = default;

    LogBackendManager(const LogBackendManager&) = delete;
    LogBackendManager& operator=(const LogBackendManager&) = delete;
    LogBackendManager(LogBackendManager&&) = delete;
    LogBackendManager& operator=(LogBackendManager&&) = delete;

    static LogBackendManager& Get()
    {
        static LogBackendManager log_backend_manager;
        return log_backend_manager;
    }

public:
    template <typename T, typename... Args>
        requires std::derived_from<T, backends::ILogBackend>
    void AddBackend(Args&&... args)
    {
        std::lock_guard lock(backends_mutex);
        backends.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void WriteToAllBackends(const LogEntry& entry)
    {
        std::lock_guard lock(backends_mutex);
        for (const auto& backend : backends)
        {
            backend->WriteLog(entry);
        }
    }

    void FlushAllBackends()
    {
        std::lock_guard lock(backends_mutex);
        for (const auto& backend : backends)
        {
            backend->Flush();
        }
    }

private:
    vector<std::unique_ptr<backends::ILogBackend>> backends;
    std::mutex backends_mutex;
};
}
