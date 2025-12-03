#include "Core/Logging/LogSettings.h"

#ifdef _WIN32  // NOLINT(readability-avoid-unconditional-preprocessor-if)
#include <Windows.h>

#else
#include <cstdlib>
#include <string_view>
#endif


namespace se::core
{
bool LogSettings::DetectColorSupport()
{
    if (ForceColor)
    {
        return true;
    }

#ifdef _WIN32
    const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(console, &mode))
    {
        return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }
    return false;
#else
    const char* term = std::getenv("TERM");
    if (term && std::string_view(term) != "dumb")
    {
        return true;
    }
    return false;
#endif
}
}
