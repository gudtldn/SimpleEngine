module SimpleEngine.App;

#define RETURN_IF_FAILED(x) if (!(x)) { return; }

import SimpleEngine.Subsystems.PlatformSubsystem;
import SimpleEngine.Logging;
import SimpleEngine.Config;

import <cassert>;
import <SDL3/SDL.h>;
import <SDL3/SDL_init.h>;
import <SDL3/SDL_video.h>;


double Application::CurrentTime = 0.0;
double Application::LastTime = 0.0;
double Application::DeltaTime = 1.0 / 60.0;
double Application::FixedDeltaTime = 1.0 / 60.0;
uint64 Application::TotalElapsedTime = 0;

uint32 Application::TargetFps = 240;
double Application::TargetFrameTime = 1.0 / static_cast<double>(TargetFps);

Application* Application::Instance = nullptr;


Application::Application(EApplicationMode in_application_mode)
    : application_mode(in_application_mode)
{
    assert(!Instance && "Application instance already exists!");
    Instance = this;
}

Application& Application::Get()
{
    assert(Instance && "Application instance is null! Startup must be called first.");
    return *Instance;
}

void Application::Startup(const char* cmd_line)
{
    Startup(se::string_utils::ToU8String(cmd_line));
}

void Application::Startup(const wchar_t* cmd_line)
{
    Startup(se::string_utils::ToU8String(cmd_line));
}

void Application::Startup(const std::u8string& cmd_line)
{
#ifdef _DEBUG
    LogSettings::EnableColor(true);
    LogSettings::SetForceColor(true);
#endif

    ConsoleLog(ELogLevel::Info, u8"startup, cmd: {}", cmd_line);

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

    ConsoleLog(ELogLevel::Info, u8"shutdown");
}

void Application::MainLoop()
{
    is_running = true;

    double performance_frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    if (performance_frequency <= 0.0)
    {
        performance_frequency = 1000.0;
    }

    CurrentTime = static_cast<double>(SDL_GetPerformanceCounter()) / performance_frequency;

    while (is_running && !quit_requested)
    {
        const double frame_start = static_cast<double>(SDL_GetPerformanceCounter()) / performance_frequency;

        // Calculate Delta Time
        LastTime = CurrentTime;
        CurrentTime = frame_start;
        DeltaTime = CurrentTime - LastTime;
        TotalElapsedTime += static_cast<uint64>(DeltaTime * 1000.0);

        ProcessPlatformEvents();

        Update(static_cast<float>(DeltaTime));

        Render();

        double frame_duration;
        do
        {
            SDL_Delay(0);
            const double frame_end = static_cast<double>(SDL_GetPerformanceCounter()) / performance_frequency;
            frame_duration = frame_end - CurrentTime;
        }
        while (frame_duration < TargetFrameTime);
    }
}

bool Application::PreInitialize()
{
    engine_instance = std::make_unique<Engine>();
    if (engine_instance == nullptr)
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to create engine instance!");
        return false;
    }
    return true;
}

void Application::RegisterSubsystems()
{
    engine_instance->RegisterSubsystem<PlatformSubsystem>();
}

bool Application::InitializeEngine()
{
    if (!engine_instance->Initialize())
    {
        ConsoleLog(ELogLevel::Error, u8"Engine failed to initialize!");
        return false;
    }
    return true;
}

bool Application::PostInitialize()
{
    const PlatformSubsystem* platform_sys = engine_instance->GetSubsystem<PlatformSubsystem>();
    platform_sys->GetEventDispatcher().Subscribe(
        EventPriority::High, [this](const PlatformEvent& event)
        {
            if (event.sdl_event.type == SDL_EVENT_QUIT)
            {
                RequestQuit();
            }
        }
    );
    return true;
}

void Application::ProcessPlatformEvents()
{
    PlatformSubsystem* platform_sys = engine_instance->GetSubsystem<PlatformSubsystem>();
    platform_sys->PollEvents();
}

void Application::Update(float delta_time)
{
    engine_instance->UpdateFrame(delta_time);
}

void Application::Render() const
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
