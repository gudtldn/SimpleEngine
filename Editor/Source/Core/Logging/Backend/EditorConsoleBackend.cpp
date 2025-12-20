#include "Core/Logging/Backend/EditorConsoleBackend.h"


namespace se::editor
{
void EditorConsoleBackend::WriteLog(const core::LogEntry& entry)
{
    std::scoped_lock lock(log_mutex);

    if (log_history.Len() >= MAX_LOG_LINES)
    {
        log_history.PopFront(); // Deque라 앞부분 삭제가 빠름
    }

    log_history.PushBack(entry);
}

void EditorConsoleBackend::Flush()
{
}

void EditorConsoleBackend::Clear()
{
    std::scoped_lock lock(log_mutex);
    log_history.Clear();
}

void EditorConsoleBackend::ReadLogs(const Function<void(const Deque<core::LogEntry>&)>& visitor)
{
    std::scoped_lock lock(log_mutex);
    if (visitor)
    {
        visitor(log_history);
    }
}
}
