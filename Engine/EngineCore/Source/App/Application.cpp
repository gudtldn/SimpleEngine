module SimpleEngine.App;

#define RETURN_IF_FAILED(x) if (!(x)) { return; }

import SimpleEngine.Subsystems.SdlSubsystem;
import SimpleEngine.Logging;
import SimpleEngine.Config;

import <cassert>;
import <SDL3/SDL.h>;
import <SDL3/SDL_init.h>;


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
    RETURN_IF_FAILED(InitializeEngine());
    RETURN_IF_FAILED(InitializeSubSystems());
    RETURN_IF_FAILED(PostInitialize());

    MainLoop();
}

void Application::Shutdown()
{
    PreRelease();
    ReleaseSubSystems();
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
    return true;
}

bool Application::InitializeEngine()
{
    engine_instance = std::make_unique<Engine>();
    if (!engine_instance->Initialize())
    {
        ConsoleLog(ELogLevel::Error, u8"Engine failed to initialize!");
        return false;
    }

    SdlSubsystem* sdl_sys = engine_instance->RegisterSubSystem<SdlSubsystem>();
    sdl_sys->SetSdlInitFlags(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);

    return true;
}

bool Application::InitializeSubSystems()
{
    if (!engine_instance->InitializeAllSubSystems())
    {
        ConsoleLog(ELogLevel::Error, u8"SubSystems failed to initialize!");
        return false;
    }
    return true;
}

bool Application::PostInitialize()
{
    using namespace se::config;

    const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();
    ParseResult result = Config::ReadConfig(solution_path / u8"Config/EngineConfig.toml");
    if (!result.has_value())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to read config file: {}", result.error().description());
        return false;
    }

    const Config& config = result.value();
    SdlSubsystem* sdl_sys = engine_instance->GetSubSystem<SdlSubsystem>();

    const std::u8string window_title = config.GetValue<std::u8string>(u8"windows.title").value_or(u8"SimpleEngine");
    const int32 window_width = config.GetValue<int32>(u8"windows.width").value_or(1280);
    const int32 window_height = config.GetValue<int32>(u8"windows.height").value_or(720);

    if (!sdl_sys->CreateWindowAndGpuDevice(window_title, window_width, window_height, SDL_WINDOW_RESIZABLE))
    {
        return false;
    }

    return true;
}

void Application::ProcessPlatformEvents()
{
    SdlSubsystem* sdl_sys = engine_instance->GetSubSystem<SdlSubsystem>();
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
    engine_instance->TickAllSubSystems(delta_time);
}

void Application::PreRelease()
{
}

void Application::ReleaseSubSystems()
{
    engine_instance->ReleaseAllSubSystems();
}

void Application::ReleaseEngine()
{
    engine_instance->Release();
}

void Application::PostRelease()
{
}
