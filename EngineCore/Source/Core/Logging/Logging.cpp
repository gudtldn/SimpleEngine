#include "Core/Logging/Logging.h"

#include <ranges>


void PrintStackTrace()
{
    if constexpr (SE_DEBUG_BUILD)
    {
        const std::stacktrace stack_trace = std::stacktrace::current();

        ConsoleLog(ELogLevel::Debug, "Stack Trace:");
        for (const std::stacktrace_entry& entry : stack_trace | std::views::drop(1) | std::views::reverse)
        {
            ConsoleLog(
                ELogLevel::Debug,
                "[{}:{}]: {}",
                entry.source_file(), entry.source_line(), entry.description()
            );
        }
    }
}
