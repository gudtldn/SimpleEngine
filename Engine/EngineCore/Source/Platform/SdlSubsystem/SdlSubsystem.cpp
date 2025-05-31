module SimpleEngine.Platform.SdlSubsystem;

import SimpleEngine.Logging;
import <SDL3/SDL_gpu.h>;


void SdlSubsystem::SetSdlInitFlags(uint32 in_sdl_init_flags)
{
    sdl_init_flags = in_sdl_init_flags;
}

bool SdlSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing SDL subsystem...");

#ifdef _DEBUG
    if (sdl_init_flags == 0)
    {
        ConsoleLog(ELogLevel::Warning, u8"SDL_InitFlags is not set! Setting to default value...");
    }
#endif

    if (!SDL_Init(sdl_init_flags))
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_Init failed: {}", SDL_GetError());
        return false;
    }
    ConsoleLog(ELogLevel::Info, u8"SDL_Init succeeded");
    return true;
}

void SdlSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, u8"Releasing SDL subsystem...");

    ConsoleLog(ELogLevel::Debug, u8"Destroying GPU device...");
    if (gpu_device)
    {
        if (window)
        {
            SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
        }
        SDL_DestroyGPUDevice(gpu_device);
        gpu_device = nullptr;
    }

    ConsoleLog(ELogLevel::Debug, u8"Destroying window...");
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SdlSubsystem::PollEvents(std::vector<SDL_Event>& out_events)
{
    out_events.clear(); // 이전 이벤트 목록 비우기
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        out_events.push_back(event);
    }
}

bool SdlSubsystem::CreateWindowAndGpuDevice(const std::u8string& window_title, uint32 window_width, uint32 window_height, uint32 sdl_window_flags)
{
    if (window)
    {
        ConsoleLog(ELogLevel::Warning, u8"Window already exists!");
        return false;
    }

    const char* window_title_c = reinterpret_cast<const char*>(window_title.c_str());
    window = SDL_CreateWindow(window_title_c, window_width, window_height, sdl_window_flags);

    if (!window)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    // 지원할 셰이더 포맷들 설정
    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METALLIB_BOOLEAN, true);

    // 디버그 모드 설정
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);

    gpu_device = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);

    if (!gpu_device)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_CreateGPUDeviceWithProperties failed: {}", SDL_GetError());
        SDL_DestroyWindow(window);
        window = nullptr;
        return false;
    }

    // 윈도우를 GPU 디바이스에 연결
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        SDL_DestroyGPUDevice(gpu_device);
        SDL_DestroyWindow(window);
        gpu_device = nullptr;
        window = nullptr;
        return false;
    }

    ConsoleLog(ELogLevel::Info, u8"Window and GPU device created successfully");
    return true;
}
