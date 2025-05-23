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
    // TODO: FORCE_INLINE 만들기
    inline static double GetCurrentTime() { return CurrentTime; }
    inline static double GetLastTime() { return LastTime; }
    inline static double GetDeltaTime() { return DeltaTime; }
    inline static double GetFixedDeltaTime() { return FixedDeltaTime; }
};
