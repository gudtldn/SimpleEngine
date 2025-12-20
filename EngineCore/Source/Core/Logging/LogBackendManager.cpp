#include "Core/Logging/LogBackendManager.h"

#include <ranges>


namespace se::core
{
void LogBackendManager::WriteToAllBackends(const LogEntry& entry)
{
    std::lock_guard lock(backends_mutex);
    for (const auto& backend : backends | std::views::values)
    {
        backend->WriteLog(entry);
    }
}

void LogBackendManager::FlushAllBackends()
{
    std::lock_guard lock(backends_mutex);
    for (const auto& backend : backends | std::views::values)
    {
        backend->Flush();
    }
}
}
