#include "UI/EditorUISubsystem.h"

#include "Panels/AssetsBrowserPanel.h"
#include "Panels/DetailPanel.h"
#include "Panels/ImGuiDemoPanel.h"
#include "Panels/OutlinerPanel.h"
#include "Panels/ViewportPanel.h"
#include "SimpleEngine/Utility/PathResolver.h"
#include "SimpleEngine/World/WorldSubsystem.h"

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

    // 한글 폰트 추가
    if (const Optional ttf_path_opt = utility::PathResolver::Get().Resolve("CoreAssets://Font/malgun.ttf"))
    {
        io.Fonts->AddFontFromFileTTF(ttf_path_opt->generic_string().c_str(), 17.0f, nullptr, io.Fonts->GetGlyphRangesKorean());
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "Failed to load font: CoreAssets://Font/malgun.ttf");
    }

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
    RegisterPanel<ImGuiDemoPanel>(refl::GetTypeName<ImGuiDemoPanel>());
    RegisterPanel<OutlinerPanel>(refl::GetTypeName<OutlinerPanel>());
    RegisterPanel<DetailPanel>(refl::GetTypeName<DetailPanel>());
    RegisterPanel<ViewportPanel>("ViewportPanel_Main", "ViewportPanel_Main");
    RegisterPanel<AssetsBrowserPanel>(refl::GetTypeName<AssetsBrowserPanel>());

    return true;
}

void EditorUISubsystem::Release()
{
    panels.Clear();

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

void EditorUISubsystem::Update(float delta_time)
{
    SetupDockSpace();
    DrawMainMenu();

    for (const auto& panel : panels | std::views::values)
    {
        if (panel->IsVisible())
        {
            panel->Draw();
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

Optional<const IEditorPanel&> EditorUISubsystem::GetPanel(const StringName& panel_id) const
{
    return panels.Find(panel_id)
        .AndThen([](const auto& panel_ptr) -> Optional<const IEditorPanel&>
        {
            return *panel_ptr;
        });
}

void EditorUISubsystem::SetupDockSpace()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
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
            for (const auto& panel : panels | std::views::values)
            {
                bool is_open = panel->IsVisible();
                if (ImGui::MenuItem(panel->GetName(), nullptr, &is_open))
                {
                    panel->SetVisibility(is_open);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity"))
        {
            if (ImGui::MenuItem("Spawn Entity"))
            {
                if (const WorldSubsystem* world_subsystem = utility::GetSubsystemUnchecked<WorldSubsystem>())
                {
                    world::World* world = world_subsystem->GetWorld();
                    world->SpawnEntity();
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
}
