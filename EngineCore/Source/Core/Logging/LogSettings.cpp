#include "Core/Logging/LogSettings.h"

#ifdef SE_PLATFORM_WINDOWS
#include <Windows.h>

#else
#include <cstdlib>
#include "Core/Container/StringView.h"
#endif


namespace se
{
bool LogSettings::DetectColorSupport()
{
    if (ForceColor)
    {
        return true;
    }

#ifdef SE_PLATFORM_WINDOWS
    const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(console, &mode))
    {
        return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
    return false;
#else
    const char* term = std::getenv("TERM");
    if (term && StringView(term) != "dumb")
    {
        return true;
    }
    return false;
#endif
}
}
