#include "Core/Logging/LogBackendManager.h"


namespace se::core
{
void LogBackendManager::WriteToAllBackends(const LogEntry& entry)
{
    std::lock_guard lock(backends_mutex);
    for (const auto& backend : backends)
    {
        backend->WriteLog(entry);
    }
}

void LogBackendManager::FlushAllBackends()
{
    std::lock_guard lock(backends_mutex);
    for (const auto& backend : backends)
    {
        backend->Flush();
    }
}
}
