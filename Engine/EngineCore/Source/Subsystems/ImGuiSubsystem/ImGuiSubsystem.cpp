module;
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
module SimpleEngine.Subsystems.ImGuiSubsystem;

import SimpleEngine.Subsystems;
import SimpleEngine.Subsystems.PlatformSubsystem;
import SimpleEngine.Subsystems.RenderSubsystem;
import std;
import <SDL3/SDL.h>;


bool ImGuiSubsystem::Initialize()
{
    const PlatformSubsystem* platform_subsystem = GetSubsystem<PlatformSubsystem>();
    RenderSubsystem* render_subsystem = GetSubsystem<RenderSubsystem>();

    // Render Subsystem에 등록
    render_subsystem->RegisterRenderableSystem(this);

    SDL_Window* window = platform_subsystem->GetWindow();
    SDL_GPUDevice* gpu_device = render_subsystem->GetGpuDevice();

    // ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& IO = ImGui::GetIO();
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    // TODO: 나중에 다중모니터 지원하도록 변경
    const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
        .Device = gpu_device,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
    };
    ImGui_ImplSDLGPU3_Init(&init_info);

    // Platform Event 등록
    platform_subsystem->GetEventDispatcher().Subscribe(
        EventPriority::High, [this](const PlatformEvent& event)
        {
            ImGui_ImplSDL3_ProcessEvent(&event.sdl_event);
        }
    );

    return true;
}

void ImGuiSubsystem::Release()
{
    RenderSubsystem* render_subsystem = GetSubsystem<RenderSubsystem>();
    render_subsystem->UnregisterRenderableSystem(this);

    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiSubsystem::Update([[maybe_unused]] float delta_time)
{
    // ImGui Update
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

std::vector<std::type_index> ImGuiSubsystem::GetDependencies() const
{
    return {
        typeid(PlatformSubsystem),
        typeid(RenderSubsystem)
    };
}
