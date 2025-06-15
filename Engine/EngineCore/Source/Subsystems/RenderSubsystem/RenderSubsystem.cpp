module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Logging;
import SimpleEngine.Subsystems;
import SimpleEngine.Subsystems.PlatformSubsystem;
import <SDL3/SDL_gpu.h>;


bool RenderSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing Render subsystem...");

    const PlatformSubsystem* platform_subsystem = GetSubsystem<PlatformSubsystem>();
    cached_window = platform_subsystem->GetWindow();

    // Window가 존재하는지 확인
    if (!cached_window)
    {
        ConsoleLog(ELogLevel::Error, u8"Window not found. Render subsystem cannot be initialized.");
        return false;
    }

    // 지원할 셰이더 포맷들 설정
    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METALLIB_BOOLEAN, true);

#ifdef _DEBUG
    // 디버그 모드 설정
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
#endif

    gpu_device = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);

    if (!gpu_device)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_CreateGPUDeviceWithProperties failed: {}", SDL_GetError());
        return false;
    }

    // Window를 GPU Device에 연결
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, cached_window))
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        SDL_DestroyGPUDevice(gpu_device);
        gpu_device = nullptr;
        return false;
    }

    if (!SDL_SetGPUSwapchainParameters(gpu_device, cached_window, swapchain_composition, present_mode))
    {
        ConsoleLog(ELogLevel::Warning, u8"SDL_SetGPUSwapchainParameters failed: {}", SDL_GetError());
    }

    ConsoleLog(ELogLevel::Info, u8"Window and GPU device created successfully");
    return true;
}

void RenderSubsystem::Release()
{
    if (gpu_device)
    {
        if (cached_window)
        {
            SDL_ReleaseWindowFromGPUDevice(gpu_device, cached_window);
        }
        SDL_DestroyGPUDevice(gpu_device);
        gpu_device = nullptr;
    }
}

std::vector<std::type_index> RenderSubsystem::GetDependencies() const
{
    return {
        typeid(PlatformSubsystem)
    };
}

void RenderSubsystem::ConfigureSwapchain(SDL_GPUSwapchainComposition in_composition, SDL_GPUPresentMode in_present_mode)
{
    swapchain_composition = in_composition;
    present_mode = in_present_mode;
}

void RenderSubsystem::RegisterRenderableSystem(IRenderable* renderable_system)
{
    renderable_systems.push_back(renderable_system);
}

void RenderSubsystem::UnregisterRenderableSystem(IRenderable* renderable_system)
{
    std::erase_if(renderable_systems, [renderable_system](const IRenderable* system) { return system == renderable_system; });
}

void RenderSubsystem::RenderFrame()
{
    // TODO: Implements this
}
