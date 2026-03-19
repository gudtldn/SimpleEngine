#include "SimpleEngine/Graphics/RenderSubsystem.h"

#include <ranges>

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_hints.h"
#include "tracy/Tracy.hpp"

using namespace se::graphics;


namespace se
{
// TODO: GameServer는 이거 필요없는데
SE_REGISTER_SUBSYSTEM(RenderSubsystem)
    .DependsOn<WindowSubsystem>();

SE_BEGIN_REFLECT(RenderSubsystem, meta::Internal)
SE_END_REFLECT(RenderSubsystem)

bool RenderSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Render subsystem...");

    const WindowSubsystem& window_subsystem = se::GetSubsystemChecked<const WindowSubsystem>();
    SDL_Window* main_window = window_subsystem.GetMainWindow();

    // Window가 존재하는지 확인
    if (!main_window)
    {
        ConsoleLog(ELogLevel::Error, "Window not found. Render subsystem cannot be initialized.");
        return false;
    }

    // 지원할 셰이더 포맷들 설정
    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METALLIB_BOOLEAN, true);

#if SE_BUILD_DEBUG
    // 디버그 모드 설정
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, true);
#endif

#if SE_PLATFORM_WINDOWS
    SDL_SetHint(SDL_HINT_GPU_DRIVER, "direct3d12");
#elif SE_PLATFORM_LINUX
    SDL_SetHint(SDL_HINT_GPU_DRIVER, "vulkan");
#elif SE_PLATFORM_MACOS
    SDL_SetHint(SDL_HINT_GPU_DRIVER, "metal");
#endif

    SDL_GPUDevice* raw_device = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);

    if (!raw_device)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUDeviceWithProperties failed: {}", SDL_GetError());
        return false;
    }

    render_device = std::make_unique<RenderDevice>(raw_device);

    // Main Window를 GPU Device에 연결
    if (!SDL_ClaimWindowForGPUDevice(render_device->GetRawDevice(), main_window))
    {
        ConsoleLog(ELogLevel::Error, "SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError());
        render_device.reset();
        return false;
    }

    const Optional window_desc_opt = window_subsystem.GetWindowDesc(window_subsystem.GetMainWindowID());
    const SDL_GPUSwapchainComposition swapchain_composition = window_desc_opt->swapchain_composition
        .ValueOr(DetermineBestSwapchainComposition(main_window, *window_desc_opt));
    const SDL_GPUPresentMode present_mode = window_desc_opt->present_mode
        .ValueOr(DetermineBestPresentMode(main_window));

    if (!SDL_SetGPUSwapchainParameters(render_device->GetRawDevice(), main_window, swapchain_composition, present_mode))
    {
        ConsoleLog(ELogLevel::Warning, "SDL_SetGPUSwapchainParameters failed: {}", SDL_GetError());
    }

    resource_manager = std::make_unique<GpuResourceManager>(*render_device);
    render_graph = std::make_unique<RenderGraph>(*render_device);
    pso_manager = std::make_unique<PSOManager>(*render_device);

    // 동적 윈도우 생성/파괴에 대응하기 위해 Delegate 구독
    WindowSubsystem& window_subsystem_mut = se::GetSubsystemChecked<WindowSubsystem>();
    window_created_handle = window_subsystem_mut.on_window_created.AddLambda(
        [this](SDL_WindowID window_id, SDL_Window* window, const WindowDesc& desc)
        {
            OnWindowCreated(window_id, window, desc);
        });
    window_destroyed_handle = window_subsystem_mut.on_window_destroyed.AddLambda(
        [this](SDL_WindowID window_id, SDL_Window* window)
        {
            OnWindowDestroyed(window_id, window);
        });

    ConsoleLog(ELogLevel::Info, "Window and GPU device created successfully");
    return true;
}

void RenderSubsystem::Release()
{
    if (!render_device)
    {
        return;
    }

    // Delegate 구독 해제
    WindowSubsystem* window_subsystem = se::GetSubsystem<WindowSubsystem>();
    if (window_subsystem)
    {
        if (window_created_handle.IsValid())
        {
            window_subsystem->on_window_created.Remove(window_created_handle);
            window_created_handle.Invalidate();
        }
        if (window_destroyed_handle.IsValid())
        {
            window_subsystem->on_window_destroyed.Remove(window_destroyed_handle);
            window_destroyed_handle.Invalidate();
        }
    }

    render_graph.reset();
    pso_manager.reset();
    resource_manager.reset();

    // 모든 윈도우에서 GPU 디바이스 릴리스 (RenderDevice 소멸 전에 수행)
    if (window_subsystem)
    {
        window_subsystem->ForEachWindow([this](SDL_WindowID, SDL_Window* window, const WindowDesc&)
        {
            SDL_ReleaseWindowFromGPUDevice(render_device->GetRawDevice(), window);
        });
    }

    // RenderDevice 소멸자에서 SDL_DestroyGPUDevice를 호출
    render_device.reset();
}

void RenderSubsystem::RenderFrame() const
{
    ZoneScoped;

    const WindowSubsystem& window_subsystem = se::GetSubsystemChecked<const WindowSubsystem>();
    window_subsystem.ForEachWindow([this](SDL_WindowID, SDL_Window* window, const WindowDesc&)
    {
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            render_graph->Clear();
            return;
        }

        // Command Buffer 가져오기
        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(render_device->GetRawDevice());
        if (!command_buffer)
        {
            render_graph->Clear();

            ConsoleLog(ELogLevel::Error, "SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
            return;
        }

        // Swapchain Texture 가져오기 (화면에 그릴 캔버스 역할)
        SDL_GPUTexture* swapchain_texture;
        SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr);

        if (!swapchain_texture)
        {
            render_graph->Clear();

            SDL_CancelGPUCommandBuffer(command_buffer);
            return;
        }

        // --- Render Graph 설정 및 실행 ---

        // 스왑체인 텍스처를 "BackBuffer"라는 이름으로 RenderGraph에 임포트
        [[maybe_unused]] const RGResourceHandle backbuffer_handle = render_graph->ImportTexture("BackBuffer", swapchain_texture);

        // TODO: 여기에 다른 렌더 패스들을 추가 (예: GBuffer, 조명, UI 등).

        // 렌더 그래프를 컴파일 (의존성 분석, 리소스 생명주기 관리 등)
        render_graph->Compile();

        // 렌더 그래프를 실행하여 커맨드 버퍼에 렌더링 커맨드를 기록
        render_graph->Execute(command_buffer, GetPSOManager());

        // --- Render Graph 끝 ---

        // Command Buffer 제출
        SDL_SubmitGPUCommandBuffer(command_buffer);

        // 다음 프레임을 위해 렌더 그래프 상태를 클리어
        render_graph->Clear();
    });

    pso_manager->EndFrame();
    render_device->ProcessDeferredDestructions();
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

// ReSharper disable once CppMemberFunctionMayBeConst
void RenderSubsystem::OnWindowCreated(SDL_WindowID window_id, SDL_Window* window, const WindowDesc& desc)
{
    if (!render_device)
    {
        return;
    }

    if (!SDL_ClaimWindowForGPUDevice(render_device->GetRawDevice(), window))
    {
        ConsoleLog(ELogLevel::Warning, "SDL_ClaimWindowForGPUDevice failed for window {}: {}", window_id, SDL_GetError());
        return;
    }

    const SDL_GPUSwapchainComposition composition = desc.swapchain_composition.ValueOr(
        DetermineBestSwapchainComposition(window, desc)
    );
    const SDL_GPUPresentMode present_mode = desc.present_mode.ValueOr(DetermineBestPresentMode(window));

    if (!SDL_SetGPUSwapchainParameters(render_device->GetRawDevice(), window, composition, present_mode))
    {
        ConsoleLog(ELogLevel::Warning, "SDL_SetGPUSwapchainParameters failed for window {}: {}", window_id, SDL_GetError());
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void RenderSubsystem::OnWindowDestroyed([[maybe_unused]] SDL_WindowID window_id, SDL_Window* window)
{
    if (!render_device)
    {
        return;
    }

    SDL_ReleaseWindowFromGPUDevice(render_device->GetRawDevice(), window);
}

SDL_GPUSwapchainComposition RenderSubsystem::DetermineBestSwapchainComposition(SDL_Window* window, const WindowDesc& desc) const
{
    // HDR이 요청되고 지원되는 경우
    if (
        desc.enable_hdr
        && SDL_WindowSupportsGPUSwapchainComposition(render_device->GetRawDevice(), window, SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR)
    )
    {
        return SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR;
    }

    // 선형 색공간이 선호되고 지원되는 경우
    if (
        desc.prefer_linear_color_space
        && SDL_WindowSupportsGPUSwapchainComposition(render_device->GetRawDevice(), window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR)
    )
    {
        return SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR;
    }

    // 기본값
    return SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
}

SDL_GPUPresentMode RenderSubsystem::DetermineBestPresentMode(SDL_Window* window) const
{
    // MAILBOX가 지원되면 우선 선택 (낮은 지연시간)
    if (SDL_WindowSupportsGPUPresentMode(render_device->GetRawDevice(), window, SDL_GPU_PRESENTMODE_MAILBOX))
    {
        return SDL_GPU_PRESENTMODE_MAILBOX;
    }

    // IMMEDIATE가 지원되면 다음 선택
    if (SDL_WindowSupportsGPUPresentMode(render_device->GetRawDevice(), window, SDL_GPU_PRESENTMODE_IMMEDIATE))
    {
        return SDL_GPU_PRESENTMODE_IMMEDIATE;
    }

    // 기본값 (항상 지원됨)
    return SDL_GPU_PRESENTMODE_VSYNC;
}
}  // namespace se
