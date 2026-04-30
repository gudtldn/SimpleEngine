#pragma once
#include <mutex>

#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Logging/Backends/ILogBackend.h"


namespace se::editor
{
/**
 * @todo docs
 */
class EditorConsoleBackend : public ILogBackend
{
public:
    //~ ILogBackend
    virtual void WriteLog(const LogEntry& entry) override;
    virtual void Flush() override;
    //~ ILogBackend

    void Clear();
    void ReadLogs(const Function<void(const Deque<LogEntry>&)>& visitor) const;

    void SetMaxLogLines(usize max_lines) { max_log_lines = max_lines; }
    [[nodiscard]] usize GetMaxLogLines() const { return max_log_lines; }

private:
    usize max_log_lines = 2000;

private:
    Deque<LogEntry> log_history;
};
} // namespace se::editor
