#include "UI/EditorUISubsystem.h"
#include "Panels/ImGuiDemoPanel.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"

using namespace se::core::event;


namespace se::editor::ui
{
bool EditorUISubsystem::Initialize()
{
    const auto [platform_subsystem, render_subsystem] = GetSubsystems<PlatformSubsystem, const RenderSubsystem>();

    SDL_Window* main_window = platform_subsystem->GetMainWindow();
    SDL_GPUDevice* gpu_device = render_subsystem->GetGpuDevice();

    // ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    // TODO: 나중에 다중모니터 지원하도록 변경
    const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    ImGui_ImplSDL3_InitForSDLGPU(main_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
        .Device = gpu_device,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, main_window),
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
    };
    ImGui_ImplSDLGPU3_Init(&init_info);

    // Platform Event 등록
    platform_subsystem->GetEventDispatcher().Subscribe(
        EventPriority::High, [](const PlatformEvent& event)
        {
            ImGui_ImplSDL3_ProcessEvent(&event.sdl_event);
        }
    );

    // 일단 명시적으로 Register 코드 작성
    RegisterPanel<ImGuiDemoPanel>();

    return true;
}

void EditorUISubsystem::Release()
{
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void EditorUISubsystem::PreUpdate()
{
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorUISubsystem::Update([[maybe_unused]] float delta_time)
{
    const EditorUIContext context = {
        .delta_time = delta_time,
    };

    SetupDockSpace();
    DrawMainMenu();

    for (const auto& panel : panels)
    {
        if (panel->IsVisible())
        {
            panel->Draw(context);
        }
    }
}

void EditorUISubsystem::PostUpdate()
{
    ImGui::EndFrame();

    // Update and Render additional Platform Windows
    const ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
    }
}

void EditorUISubsystem::SetupDockSpace()
{
    ImGui::DockSpaceOverViewport();
}

void EditorUISubsystem::DrawMainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                app::Application::Get().RequestQuit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            for (const auto& panel : panels)
            {
                bool is_open = panel->IsVisible();
                if (ImGui::MenuItem(panel->GetName(), nullptr, &is_open))
                {
                    panel->SetVisibility(is_open);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
}
