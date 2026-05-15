#include "SimpleEngine/App/Application.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/HAL/CpuFeature.h"
#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Logging/LogBackendManager.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Logging/LogSettings.h"
#include "SimpleEngine/Core/Logging/Backends/ConsoleBackend.h"
#include "SimpleEngine/Core/Logging/Backends/FileBackend.h"
#include "SimpleEngine/Utility/Debug.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "SDL3/SDL.h"
#include "tracy/Tracy.hpp"

#define RETURN_IF_FAILED(x) do { if (!(x)) { ConsoleLog(ELogLevel::Error, "Initialize Failed!: {}", #x); return; } } while (false);


namespace se
{
u32 Application::target_fps = 240;
f64 Application::target_frame_time = 1.0 / static_cast<f64>(target_fps);

f64 Application::busy_wait_ratio = 0.1;
f64 Application::busy_wait_threshold = target_frame_time * busy_wait_ratio;

Application* Application::instance = nullptr;


Application::Application(EApplicationMode in_application_mode)
    : application_mode(in_application_mode)
{
    SE_ASSERT(!instance, "Application instance already exists!");
    instance = this;
}

Application::~Application()
{
    instance = nullptr;
}

Application& Application::Get()
{
    SE_ASSERT(instance, "Application instance is null! Startup must be called first.");
    return *instance;
}

void Application::Startup(const wchar_t* cmd_line)
{
    Startup(StringUtils::ToString(cmd_line));
}

void Application::Startup(const String& cmd_line)
{
    Platform::SetCurrentThreadName("Main Thread");

    if constexpr (SE_BUILD_DEBUG)
    {
        LogSettings::EnableColor(true);
        LogSettings::SetForceColor(true);
    }

    // add log backends
    {
        LogBackendManager& manager = LogBackendManager::Get();

        manager.AddBackend<ConsoleBackend>();
        manager.AddBackend<FileBackend>();
    }

    ConsoleLog(ELogLevel::Info, "startup, cmd: {}", cmd_line);

    {
        using PairType = std::pair<bool, String>;

        Array<PairType> features{
            { CpuFeature::HasSSE(), "SSE" },
            { CpuFeature::HasSSE2(), "SSE2" },
            { CpuFeature::HasSSE3(), "SSE3" },
            { CpuFeature::HasSSSE3(), "SSSE3" },
            { CpuFeature::HasSSE4_1(), "SSE4.1" },
            { CpuFeature::HasSSE4_2(), "SSE4.2" },
            { CpuFeature::HasFMA3(), "FMA3" },
            { CpuFeature::HasFMA4(), "FMA4" },
            { CpuFeature::HasAVX(), "AVX" },
            { CpuFeature::HasAVX2(), "AVX2" },
            { CpuFeature::HasAVX512F(), "AVX512F" },
            { CpuFeature::HasNEON(), "NEON" },
        };

        auto on_view = features
            | std::views::filter([](const PairType& p) { return p.first; })
            | std::views::transform([](const PairType& p) { return p.second.Bytes(); });

        auto off_view = features
            | std::views::filter([](const PairType& p) { return !p.first; })
            | std::views::transform([](const PairType& p) { return p.second.Bytes(); });

        ConsoleLog(ELogLevel::Debug, "CPU Features:");
        ConsoleLog(ELogLevel::Debug, "- On:  {}", StringUtils::Join(on_view, ", "));
        ConsoleLog(ELogLevel::Debug, "- Off: {}", StringUtils::Join(off_view, ", "));
    }

    CpuFeature::ValidateSimdSupport();

    RETURN_IF_FAILED(PreInitialize());
    RegisterSubsystems();
    RETURN_IF_FAILED(InitializeEngine());
    RETURN_IF_FAILED(PostInitialize());
    is_initialized = true;

    MainLoop();
}

void Application::Shutdown()
{
    PreRelease();
    ReleaseEngine();
    PostRelease();

    ConsoleLog(ELogLevel::Info, "shutdown");
}

void Application::MainLoop()
{
    is_running = true;

    static f64 frequency = static_cast<f64>(SDL_GetPerformanceFrequency());
    if (frequency <= 0.0)
    {
        frequency = 1000.0;
    }

    static auto get_performance_time = [] static -> f64
    {
        return static_cast<f64>(SDL_GetPerformanceCounter()) / frequency;
    };

    f64 current_time = get_performance_time();
    while (is_running && !quit_requested)
    {
        ZoneScoped;

        {
            ZoneScopedN("Frame Tick");

            const f64 frame_start = get_performance_time();

            // Calculate Delta Time
            const f64 last_time = current_time;
            current_time = frame_start;
            const f64 delta_time = current_time - last_time;

            ProcessPlatformEvents();

            Update(delta_time);

            Render();
        }
        {
            ZoneScopedN("Frame Wait");

            const f64 elapsed_sec = get_performance_time() - current_time;
            const f64 time_to_wait_sec = target_frame_time - elapsed_sec;

            if (time_to_wait_sec > 0.0)
            {
                const f64 busy_wait_threshold_ms = busy_wait_threshold * 1000.0;
                const u32 sleep_ms = static_cast<u32>((time_to_wait_sec * 1000.0) - busy_wait_threshold_ms);

                // 대부분의 대기 시간을 Delay로 대기
                if (sleep_ms > 0)
                {
                    SDL_Delay(sleep_ms);
                }

                // 남은 시간은 바쁜 대기로 대기
                while (get_performance_time() - current_time < target_frame_time){}
            }
        }

        FrameMark;
    }
}

bool Application::PreInitialize()
{
    engine_instance = std::make_unique<Engine>();
    if (engine_instance == nullptr)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create engine instance!");
        return false;
    }
    return true;
}

void Application::RegisterSubsystems()
{
    engine_instance->LoadRegisteredSubsystems();
}

bool Application::InitializeEngine()
{
    if (!engine_instance->Initialize())
    {
        ConsoleLog(ELogLevel::Error, "Engine failed to initialize!");
        return false;
    }
    return true;
}

bool Application::PostInitialize()
{
    // 이벤트 핸들러 등록
    if (EventSubsystem* event_sys = engine_instance->GetSubsystem<EventSubsystem>())
    {
        event_sys->on_quit_requested.AddLambda([this]
        {
            RequestQuit();
        });
    }

    // 윈도우 닫기 이벤트 핸들러 등록
    if (WindowSubsystem* window_sys = engine_instance->GetSubsystem<WindowSubsystem>())
    {
        window_sys->on_window_close_requested.AddLambda([this, window_sys](SDL_WindowID window_id)
        {
            if (window_id == window_sys->GetMainWindowID())
            {
                RequestQuit();
            }
            else
            {
                window_sys->DestroyWindow(window_id);
            }
        });
    }

    return true;
}

void Application::ProcessPlatformEvents()
{
    // 입력 상태 프레임 초기화 (PollEvents 전에 호출되어야 함)
    if (InputSubsystem* input_sys = engine_instance->GetSubsystem<InputSubsystem>())
    {
        input_sys->BeginFrame();
    }

    EventSubsystem* event_sys = engine_instance->GetSubsystem<EventSubsystem>();
    event_sys->PollEvents();
}

void Application::Update(f64 delta_time)
{
    engine_instance->UpdateFrame(delta_time);

    if (AssetSubsystem* asset = engine_instance->GetSubsystem<AssetSubsystem>())
    {
        asset->EndFrame();
    }
}

void Application::Render()
{
}

void Application::PreRelease()
{
}

void Application::ReleaseEngine()
{
    if (is_initialized)
    {
        engine_instance->Release();
    }
}

void Application::PostRelease()
{
}
} // namespace se
