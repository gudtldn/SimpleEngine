module;
#include "Platform/PlatformMacros.h"
export module SimpleEngine.App;


export class Application
{
private:
    static double CurrentTime;
    static double LastTime;
    static double DeltaTime;
    static double FixedDeltaTime;

public:
    Application() = default;

    // TODO: wchar_t 플랫폼별 타입으로 변경
    static void Startup(const wchar_t* cmd_line);
    static void Shutdown();

public:
    FORCE_INLINE static double GetCurrentTime() { return CurrentTime; }
    FORCE_INLINE static double GetLastTime() { return LastTime; }
    FORCE_INLINE static double GetDeltaTime() { return DeltaTime; }
    FORCE_INLINE static double GetFixedDeltaTime() { return FixedDeltaTime; }
};
