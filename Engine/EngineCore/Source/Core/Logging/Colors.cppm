export module SimpleEngine.Core:Logging.Colors;

import SimpleEngine.Types;


namespace LogColors
{
constexpr const char8* COLOR_DEBUG = u8"\x1b[36m";   // Cyan
constexpr const char8* COLOR_INFO = u8"\x1b[32m";    // Green
constexpr const char8* COLOR_WARNING = u8"\x1b[33m"; // Yellow
constexpr const char8* COLOR_ERROR = u8"\x1b[31m";   // Red
constexpr const char8* COLOR_FATAL = u8"\x1b[35m";   // Magenta
constexpr const char8* COLOR_RESET = u8"\x1b[0m";    // Reset
}
