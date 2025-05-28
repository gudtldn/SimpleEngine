module;
#include <Windows.h>
export module SimpleEngine.Logging:Colors;

import SimpleEngine.Platform.Types;
import std;


namespace LogColors
{
constexpr const char8* COLOR_DEBUG = u8"\x1b[36m";   // Cyan
constexpr const char8* COLOR_INFO = u8"\x1b[32m";    // Green
constexpr const char8* COLOR_WARNING = u8"\x1b[33m"; // Yellow
constexpr const char8* COLOR_ERROR = u8"\x1b[31m";   // Red
constexpr const char8* COLOR_FATAL = u8"\x1b[35m";   // Magenta
constexpr const char8* COLOR_RESET = u8"\x1b[0m";    // Reset
}

// 색상 설정 관리 클래스
export class LogSettings
{
private:
    static inline bool ColorEnabled = true;
    static inline bool ForceColor = false;

public:
    static void EnableColor(bool enable) { ColorEnabled = enable; }
    static void SetForceColor(bool force) { ForceColor = force; }
    static bool IsColorEnabled() { return ColorEnabled; }
    static bool IsColorForced() { return ForceColor; }

    static bool DetectColorSupport()
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
};
