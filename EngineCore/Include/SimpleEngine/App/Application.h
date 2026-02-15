#pragma once
#include <algorithm>
#include <atomic>
#include <memory>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
// forward declaration
class Engine;

enum class EApplicationMode : uint8
{
    GameClient,
    Editor,
    GameServer,
};

/**
 * 애플리케이션의 전체 수명 주기와 전역 상태를 관리하는 기본 클래스
 */
class SE_CORE_API Application
{
public:
    explicit Application(EApplicationMode in_application_mode = EApplicationMode::GameClient);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    static Application& Get();

public:
    // String으로 변경해서 호출해주는 오버로딩 함수
    virtual void Startup(const wchar_t* cmd_line);
    virtual void Startup(const String& cmd_line);

    virtual void Shutdown();

private:
    void MainLoop();

public:
    [[nodiscard]] bool IsInitialized() const { return is_initialized; }
    [[nodiscard]] bool IsRunning() const { return is_running; }

    // Application 종료 관련
    void RequestQuit() { quit_requested = true; }
    [[nodiscard]] bool IsQuitRequested() const { return quit_requested; }

    /**
     * 애플리케이션이 실행 중인 모드를 지정하는 현재 애플리케이션 모드를 가져옵니다.
     *
     * @return 현재 애플리케이션 모드는 EApplicationMode 열거형 값으로 표시됩니다.
     */
    [[nodiscard]] EApplicationMode GetApplicationMode() const { return application_mode; }

protected:
    // 초기화 단계
    virtual bool PreInitialize();
    virtual void RegisterSubsystems();
    virtual bool InitializeEngine();
    virtual bool PostInitialize();

    // 메인 루프의 각 단계
    virtual void ProcessPlatformEvents();
    virtual void Update(float delta_time);

    virtual void Render();

    // 종료 단계
    virtual void PreRelease();
    virtual void ReleaseEngine();
    virtual void PostRelease();

public:
    [[nodiscard]] static double GetCurrentTime() { return CurrentTime; }
    [[nodiscard]] static double GetLastTime() { return LastTime; }
    [[nodiscard]] static double GetDeltaTime() { return DeltaTime; }
    [[nodiscard]] static double GetFixedDeltaTime() { return FixedDeltaTime; }
    [[nodiscard]] static uint64 GetTotalElapsedTime() { return TotalElapsedTime; }

    [[nodiscard]] static uint32 GetTargetFps()
    {
        return TargetFps;
    }

    static void SetTargetFps(uint32 new_fps)
    {
        TargetFps = new_fps;
        TargetFrameTime = 1.0 / static_cast<double>(TargetFps);
        BusyWaitThreshold = TargetFrameTime * BusyWaitRatio;
    }

    [[nodiscard]] static double GetBusyWaitRatio()
    {
        return BusyWaitRatio;
    }

    static void SetBusyWaitRatio(double new_ratio)
    {
        BusyWaitRatio = std::clamp(new_ratio, 0.0, 1.0);
        BusyWaitThreshold = TargetFrameTime * BusyWaitRatio;
    }

protected:
    std::unique_ptr<Engine> engine_instance;
    EApplicationMode application_mode;

private:
    static Application* Instance;

    // 아래 시간들의 단위는 초단위
    static double CurrentTime;      // 현재 프레임 시작 시간
    static double LastTime;         // 이전 프레임 시작 시간
    static double DeltaTime;        // CurrentTime - LastTime
    static double FixedDeltaTime;   // 물리 계산용 DeltaTime
    static uint64 TotalElapsedTime; // 총 경과 시간 ms

    static uint32 TargetFps;       // 목표 FPS
    static double TargetFrameTime; // 목표 FPS 시간

    static double BusyWaitRatio;     // 바쁜 대기 시간 비율
    static double BusyWaitThreshold; // 바쁜 대기 시간 제한

    // Loop 제어 변수
    bool is_initialized = false;
    bool is_running = false;
    std::atomic<bool> quit_requested = false;
};
}  // namespace se
