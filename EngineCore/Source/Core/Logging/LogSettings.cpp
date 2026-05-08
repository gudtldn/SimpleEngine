#include "SimpleEngine/Core/Logging/LogSettings.h"

#ifdef SE_PLATFORM_WINDOWS
    #include <Windows.h>
#else
    #include "SimpleEngine/Core/Container/StringView.h"
    #include <cstdlib>
#endif


namespace se
{
bool LogSettings::DetectColorSupport()
{
    static bool supported = []
    {
#ifdef SE_PLATFORM_WINDOWS
        const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD mode = 0;
        if (GetConsoleMode(console, &mode))
        {
            return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }
        return false;
#else
        const char* term = std::getenv("TERM");
        if (term && StringView{ term } != "dumb")
        {
            return true;
        }
        return false;
#endif
    }();

    return force_color || supported;
}
} // namespace se
