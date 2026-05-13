#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <algorithm>
#include <atomic>
#include <memory>


namespace se
{
// forward declaration
class Engine;

enum class EApplicationMode : u8
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

    /** 애플리케이션이 실행 중인 모드를 지정하는 현재 애플리케이션 모드를 가져옵니다. */
    [[nodiscard]] EApplicationMode GetApplicationMode() const { return application_mode; }

protected:
    // 초기화 단계
    virtual bool PreInitialize();
    virtual void RegisterSubsystems();
    virtual bool InitializeEngine();
    virtual bool PostInitialize();

    // 메인 루프의 각 단계
    virtual void ProcessPlatformEvents();
    virtual void Update(f64 delta_time);

    virtual void Render();

    // 종료 단계
    virtual void PreRelease();
    virtual void ReleaseEngine();
    virtual void PostRelease();

public:
    [[nodiscard]] static u32 GetTargetFps()
    {
        return target_fps;
    }

    static void SetTargetFps(u32 new_fps)
    {
        target_fps = new_fps;
        target_frame_time = 1.0 / static_cast<f64>(target_fps);
        busy_wait_threshold = target_frame_time * busy_wait_ratio;
    }

    [[nodiscard]] static f64 GetBusyWaitRatio()
    {
        return busy_wait_ratio;
    }

    static void SetBusyWaitRatio(f64 new_ratio)
    {
        busy_wait_ratio = std::clamp(new_ratio, 0.0, 1.0);
        busy_wait_threshold = target_frame_time * busy_wait_ratio;
    }

protected:
    std::unique_ptr<Engine> engine_instance;
    EApplicationMode application_mode;

private:
    static Application* instance;

    // 프레임 pacing 정책
    static u32 target_fps;       // 목표 FPS
    static f64 target_frame_time; // 목표 FPS 시간

    static f64 busy_wait_ratio;     // 바쁜 대기 시간 비율
    static f64 busy_wait_threshold; // 바쁜 대기 시간 제한

    // Loop 제어 변수
    bool is_initialized = false;
    bool is_running = false;
    std::atomic<bool> quit_requested = false;
};
} // namespace se
