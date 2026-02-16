#include "SimpleEngine/App/Application.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/HAL/CpuFeature.h"
#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
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
double Application::CurrentTime = 0.0;
double Application::LastTime = 0.0;
double Application::DeltaTime = 1.0 / 60.0;
double Application::FixedDeltaTime = 1.0 / 60.0;
uint64 Application::TotalElapsedTime = 0;

uint32 Application::TargetFps = 240;
double Application::TargetFrameTime = 1.0 / static_cast<double>(TargetFps);

double Application::BusyWaitRatio = 0.1;
double Application::BusyWaitThreshold = TargetFrameTime * BusyWaitRatio;

Application* Application::Instance = nullptr;


Application::Application(EApplicationMode in_application_mode)
    : application_mode(in_application_mode)
{
    SE_ASSERT(!Instance, "Application instance already exists!");
    Instance = this;
}

Application::~Application()
{
    Instance = nullptr;
}

Application& Application::Get()
{
    SE_ASSERT(Instance, "Application instance is null! Startup must be called first.");
    return *Instance;
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

    static double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    if (frequency <= 0.0)
    {
        frequency = 1000.0;
    }

    static auto get_performance_time = [] static -> double
    {
        return static_cast<double>(SDL_GetPerformanceCounter()) / frequency;
    };

    CurrentTime = get_performance_time();

    // ReSharper disable once CppDFAConstantConditions
    while (is_running && !quit_requested)
    {
        ZoneScoped;

        {
            ZoneScopedN("Frame Tick");

            const double frame_start = get_performance_time();

            // Calculate Delta Time
            LastTime = CurrentTime;
            CurrentTime = frame_start;
            DeltaTime = CurrentTime - LastTime;
            TotalElapsedTime += static_cast<uint64>(DeltaTime * 1000.0);

            ProcessPlatformEvents();

            Update(static_cast<float>(DeltaTime));

            Render();
        }
        {
            ZoneScopedN("Frame Wait");

            const double elapsed_sec = get_performance_time() - CurrentTime;
            const double time_to_wait_sec = TargetFrameTime - elapsed_sec;

            if (time_to_wait_sec > 0.0)
            {
                const double busy_wait_threshold_ms = BusyWaitThreshold * 1000.0;
                const uint32 sleep_ms = static_cast<uint32>((time_to_wait_sec * 1000.0) - busy_wait_threshold_ms);

                // 대부분의 대기 시간을 Delay로 대기
                if (sleep_ms > 0)
                {
                    SDL_Delay(sleep_ms);
                }

                // 남은 시간은 바쁜 대기로 대기
                // ReSharper disable once CppEnforceWhileStatementBraces
                while (get_performance_time() - CurrentTime < TargetFrameTime);
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
    PlatformSubsystem* platform_sys = engine_instance->GetSubsystem<PlatformSubsystem>();

    platform_sys->on_quit_requested.AddLambda([this]
    {
        RequestQuit();
    });

    platform_sys->on_window_close_requested.AddLambda([this, platform_sys](SDL_WindowID window_id)
    {
        if (window_id == platform_sys->GetMainWindowID())
        {
            RequestQuit();
        }
        else
        {
            platform_sys->DestroyWindow(window_id);
        }
    });

    return true;
}

void Application::ProcessPlatformEvents()
{
    // 입력 상태 프레임 초기화 (PollEvents 전에 호출되어야 함)
    if (InputSubsystem* input_sys = engine_instance->GetSubsystem<InputSubsystem>())
    {
        input_sys->BeginFrame();
    }

    PlatformSubsystem* platform_sys = engine_instance->GetSubsystem<PlatformSubsystem>();
    platform_sys->PollEvents();
}

void Application::Update(float delta_time)
{
    engine_instance->UpdateFrame(delta_time);
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
}  // namespace se
