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
class EditorConsoleBackend : public core::ILogBackend
{
public:
    //~ ILogBackend
    virtual void WriteLog(const core::LogEntry& entry) override;
    virtual void Flush() override;
    //~ ILogBackend

    void Clear();
    void ReadLogs(const Function<void(const Deque<core::LogEntry>&)>& visitor) const;

private:
    static constexpr usize MAX_LOG_LINES = 2000;

private:
    Deque<core::LogEntry> log_history;
};
}  // namespace se::editor
