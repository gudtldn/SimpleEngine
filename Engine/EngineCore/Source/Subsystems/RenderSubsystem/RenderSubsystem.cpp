module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Core;
import SimpleEngine.Utility;
import SimpleEngine.Platform;
import SimpleEngine.Subsystems.Utility;
import SimpleEngine.Rendering;

import <SDL3/SDL_gpu.h>;
import <SDL3/SDL_hints.h>;


bool RenderSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing Render subsystem...");

    const PlatformSubsystem* platform_subsystem = GetSubsystem<PlatformSubsystem>();
    SDL_Window* main_window = platform_subsystem->GetMainWindow();

    // Window가 존재하는지 확인
    if (!main_window)
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

    if constexpr (se::utility::IS_DEBUG_BUILD)
    {
        // 디버그 모드 설정
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
    }

    if constexpr (se::platform::detection::IS_PLATFORM_WINDOWS)
    {
        SDL_SetHint(SDL_HINT_GPU_DRIVER, "direct3d12");
    }
    else if constexpr (se::platform::detection::IS_PLATFORM_LINUX)
    {
        SDL_SetHint(SDL_HINT_GPU_DRIVER, "vulkan");
    }
    else if constexpr (se::platform::detection::IS_PLATFORM_MAC_OS)
    {
        SDL_SetHint(SDL_HINT_GPU_DRIVER, "metal");
    }

    gpu_device = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);

    if (!gpu_device)
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_CreateGPUDeviceWithProperties failed: {}", SDL_GetError());
        return false;
    }

    // Window를 GPU Device에 연결
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, main_window))
    {
        ConsoleLog(ELogLevel::Error, u8"SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        SDL_DestroyGPUDevice(gpu_device);
        gpu_device = nullptr;
        return false;
    }

    const WindowDesc& window_desc = *platform_subsystem->GetMainWindowInfo();
    const SDL_GPUSwapchainComposition swapchain_composition = window_desc.swapchain_composition.ValueOr(
        DetermineBestSwapchainComposition(main_window, window_desc)
    );
    const SDL_GPUPresentMode present_mode = window_desc.present_mode.ValueOr(DetermineBestPresentMode(main_window));

    if (!SDL_SetGPUSwapchainParameters(gpu_device, main_window, swapchain_composition, present_mode))
    {
        ConsoleLog(ELogLevel::Warning, u8"SDL_SetGPUSwapchainParameters failed: {}", SDL_GetError());
    }

    render_graph = std::make_unique<RenderGraph>(gpu_device);
    pso_manager = std::make_unique<PSOManager>(gpu_device);

    ConsoleLog(ELogLevel::Info, u8"Window and GPU device created successfully");
    return true;
}

void RenderSubsystem::Release()
{
    if (!gpu_device)
    {
        return;
    }

    render_graph.reset();
    pso_manager.reset();

    PlatformSubsystem* platform_subsystem = GetSubsystem<PlatformSubsystem>();
    for (SDL_Window* window : platform_subsystem->GetWindows() | std::views::values)
    {
        SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    }
    SDL_DestroyGPUDevice(gpu_device);
    gpu_device = nullptr;
}

void RenderSubsystem::RenderFrame() const
{
    PlatformSubsystem* platform_subsystem = GetSubsystem<PlatformSubsystem>();
    for (SDL_Window* window : platform_subsystem->GetWindows() | std::views::values)
    {
        // Command Buffer 가져오기
        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);
        if (!command_buffer)
        {
            ConsoleLog(ELogLevel::Error, u8"SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
            return;
        }

        // Swapchain Texture 가져오기 (화면에 그릴 캔버스 역할)
        SDL_GPUTexture* swapchain_texture;
        SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr);

        if (!swapchain_texture)
        {
            SDL_CancelGPUCommandBuffer(command_buffer);
            return;
        }

        // --- Render Graph 설정 및 실행 ---

        // 스왑체인 텍스처를 "Backbuffer"라는 이름으로 RenderGraph에 임포트
        const RGResourceHandle backbuffer_handle = render_graph->ImportTexture(u8"Backbuffer", swapchain_texture);

        // ClearPass를 추가하여 Backbuffer를 회색으로 클리어
        render_graph->AddPass<ClearPass>(
            backbuffer_handle,
            SDL_FColor{ 0.25f, 0.25f, 0.25f, 1.0f }
        );

        // TODO: 여기에 다른 렌더 패스들을 추가 (예: GBuffer, 조명, UI 등).

        // 렌더 그래프를 컴파일 (의존성 분석, 리소스 생명주기 관리 등)
        render_graph->Compile();

        // 렌더 그래프를 실행하여 커맨드 버퍼에 렌더링 커맨드를 기록
        render_graph->Execute(command_buffer, *GetPSOManager());

        // --- Render Graph 끝 ---

        // Command Buffer 제출
        SDL_SubmitGPUCommandBuffer(command_buffer);

        // 다음 프레임을 위해 렌더 그래프 상태를 클리어
        render_graph->Clear();
    }

    pso_manager->EndFrame();
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

SDL_GPUSwapchainComposition RenderSubsystem::DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const
{
    // HDR이 요청되고 지원되는 경우
    if (desc.enable_hdr && SDL_WindowSupportsGPUSwapchainComposition(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR))
    {
        return SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR;
    }

    // 선형 색공간이 선호되고 지원되는 경우
    if (desc.prefer_linear_color_space && SDL_WindowSupportsGPUSwapchainComposition(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR))
    {
        return SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR;
    }

    // 기본값
    return SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
}

SDL_GPUPresentMode RenderSubsystem::DetermineBestPresentMode(SDL_Window* window) const
{
    // MAILBOX가 지원되면 우선 선택 (낮은 지연시간)
    if (SDL_WindowSupportsGPUPresentMode(gpu_device, window, SDL_GPU_PRESENTMODE_MAILBOX))
    {
        return SDL_GPU_PRESENTMODE_MAILBOX;
    }

    // IMMEDIATE가 지원되면 다음 선택
    if (SDL_WindowSupportsGPUPresentMode(gpu_device, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
    {
        return SDL_GPU_PRESENTMODE_IMMEDIATE;
    }

    // 기본값 (항상 지원됨)
    return SDL_GPU_PRESENTMODE_VSYNC;
}
