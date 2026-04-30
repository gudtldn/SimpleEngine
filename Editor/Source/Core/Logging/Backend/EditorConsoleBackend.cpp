#include "Core/Logging/Backend/EditorConsoleBackend.h"


namespace se::editor
{
void EditorConsoleBackend::WriteLog(const LogEntry& entry)
{
    if (log_history.Len() >= max_log_lines)
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
    log_history.Clear();
}

void EditorConsoleBackend::ReadLogs(const Function<void(const Deque<LogEntry>&)>& visitor) const
{
    if (visitor)
    {
        visitor(log_history);
    }
}
} // namespace se::editor
