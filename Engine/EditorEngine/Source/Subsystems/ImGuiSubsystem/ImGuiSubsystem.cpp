module;
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
module SimpleEngine.Editor.Subsystems.ImGuiSubsystem;

import SimpleEngine.Subsystems.Utility;
import std;
import <SDL3/SDL.h>;


bool ImGuiSubsystem::Initialize()
{
    auto [platform_subsystem, render_subsystem] = GetSubsystems<const PlatformSubsystem, const RenderSubsystem>();

    SDL_Window* main_window = platform_subsystem->GetMainWindow();
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

    ImGui_ImplSDL3_InitForSDLGPU(main_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
        .Device = gpu_device,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, main_window),
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
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
