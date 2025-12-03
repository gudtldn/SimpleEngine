#pragma once
#include <concepts>
#include <memory>
#include <mutex>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Logging/LogData.h"
#include "SimpleEngine/Core/Logging/Backends/ILogBackend.h"

#include "tracy/Tracy.hpp"


namespace se::core
{
class SE_CORE_API LogBackendManager
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
        requires std::derived_from<T, ILogBackend>
    void AddBackend(Args&&... args)
    {
        std::lock_guard lock(backends_mutex);
        backends.Emplace(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void WriteToAllBackends(const LogEntry& entry);
    void FlushAllBackends();

private:
    Array<std::unique_ptr<ILogBackend>> backends{};
    TracyLockable(std::mutex, backends_mutex);
};
}
