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

void RenderSubsystem::RenderFrame() const
{
    // TODO: 렌더링 순서 생각해야함

    // Command Buffer 가져오기
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
    if (!command_buffer)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
        return;
    }

    // Swapchain Texture 가져오기 (화면에 그릴 캔버스 역할)
    SDL_GPUTexture* swapchain_texture;
    SDL_AcquireGPUSwapchainTexture(command_buffer, cached_window, &swapchain_texture, nullptr, nullptr);

    if (swapchain_texture)
    {
        constexpr SDL_FColor clear_color = { 0.25f, 0.25f, 0.25f, 1.0f };

        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        target_info.clear_color = clear_color;
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;

        // TODO: IRenderPass를 돌아가면서 렌더링 하도록 하기
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
        {
            // TODO: 등록된 모든 렌더러들의 Render() 함수를 호출하여 그리게 함
            // RenderFrameContext context{ .CommandBuffer = command_buffer };
            // for (IRenderer* renderer : registered_renderers)
            // {
            //     renderer->Render(context);
            // }
        }
        SDL_EndGPURenderPass(render_pass);
    }

    // Command Buffer 제출
    SDL_SubmitGPUCommandBuffer(command_buffer);
}

void RenderSubsystem::BeginFrame() const
{
}

void RenderSubsystem::EndFrame() const
{
}

void RenderSubsystem::SubmitCommands() const
{
}
