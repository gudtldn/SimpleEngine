#include "SimpleEngine/Core/Logging/LogBackendManager.h"

#include <ranges>


namespace se
{
void LogBackendManager::WriteToAllBackends(const LogEntry& entry)
{
    std::scoped_lock lock(backends_mutex);
    for (const auto& backend : backends | std::views::values)
    {
        backend->WriteLog(entry);
    }
}

void LogBackendManager::FlushAllBackends()
{
    std::scoped_lock lock(backends_mutex);
    for (const auto& backend : backends | std::views::values)
    {
        backend->Flush();
    }
}
}  // namespace se
