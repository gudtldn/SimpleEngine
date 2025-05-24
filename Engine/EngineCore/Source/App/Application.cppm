module;
#include "Platform/PlatformMacros.h"
export module SimpleEngine.App;
import SimpleEngine.Core;

/**
 * 애플리케이션의 전체 수명 주기와 전역 상태를 관리하는 기본 클래스
 */
export class Application
{
public:
    Application() = default;

    static void Startup(const char8* cmd_line);
    static void Shutdown();

public:
    FORCE_INLINE static double GetCurrentTime() { return CurrentTime; }
    FORCE_INLINE static double GetLastTime() { return LastTime; }
    FORCE_INLINE static double GetDeltaTime() { return DeltaTime; }
    FORCE_INLINE static double GetFixedDeltaTime() { return FixedDeltaTime; }

private:
    // 아래 시간들의 단위는 초단위
    static double CurrentTime;
    static double LastTime;
    static double DeltaTime;
    static double FixedDeltaTime;
};
