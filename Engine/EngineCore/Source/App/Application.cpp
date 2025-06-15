module SimpleEngine.App;

#define RETURN_IF_FAILED(x) if (!(x)) { return; }

import SimpleEngine.Subsystems.SdlSubsystem;
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

        PreRender();
        Render();
        PostRender();

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

void Application::RegisterSubsystems()
{
    SdlSubsystem* sdl_sys = engine_instance->RegisterSubsystem<SdlSubsystem>();
    sdl_sys->SetSdlInitFlags(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);
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
    using namespace se::config;

    const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();
    const std::filesystem::path config_path = solution_path / u8"Config/EngineConfig.toml";

    ParseResult result = Config::ReadConfig(config_path);
    if (!result.has_value())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to read config file: {}", result.error().description());
        return false;
    }

    Config& config = result.value();
    SdlSubsystem* sdl_sys = engine_instance->GetSubsystem<SdlSubsystem>();

    const std::u8string window_title = config.GetValueOrStore<std::u8string>(u8"window.title", u8"SimpleEngine");
    const int32 window_width = config.GetValueOrStore<int32>(u8"window.width", 1280);
    const int32 window_height = config.GetValueOrStore<int32>(u8"window.height", 720);
    if (!config.WriteConfig(config_path))
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to write config file: {}", config_path.generic_u8string());
        return false;
    }

    if (!sdl_sys->CreateWindowAndGpuDevice(window_title, window_width, window_height, SDL_WINDOW_RESIZABLE))
    {
        return false;
    }
    SDL_SetWindowPosition(sdl_sys->GetWindow(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(sdl_sys->GetWindow());

    return true;
}

void Application::ProcessPlatformEvents()
{
    SdlSubsystem* sdl_sys = engine_instance->GetSubsystem<SdlSubsystem>();
    std::vector<SDL_Event> events;
    sdl_sys->PollEvents(events);

    for (const SDL_Event& event : events)
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        {
            RequestQuit();
            break;
        }
        default:
            break;
        }
    }
}

void Application::Update(float delta_time)
{
    engine_instance->UpdateAllSubsystems(delta_time);
}

void Application::PreRender()
{
}

void Application::Render()
{
}

void Application::PostRender()
{
}

void Application::PreRelease()
{
}

void Application::ReleaseEngine()
{
    engine_instance->Release();
}

void Application::PostRelease()
{
}
