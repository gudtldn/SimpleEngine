module;
#ifdef _WIN32  // NOLINT(readability-avoid-unconditional-preprocessor-if)
#include <Windows.h>
#endif
export module SE.Core:Logging.LogSettings;


/**
 * 색상 설정 관리 클래스
 */
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
