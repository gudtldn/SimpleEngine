#include "SimpleEditor/UI/EditorUISubsystem.h"

#include "Panels/AssetsBrowserPanel.h"
#include "Panels/CameraPanel.h"
#include "Panels/DebugPanel.h"
#include "Panels/DetailPanel.h"
#include "Panels/EditorConsolePanel.h"
#include "Panels/ImGuiDemoPanel.h"
#include "Panels/OutlinerPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/ViewportPanel.h"
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/Core/EditorSubsystem.h"

#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/HAL/FileDialog.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/ECS/WorldSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorUISubsystem)
    .DependsOn<
        EventSubsystem,
        WindowSubsystem,
        RenderSubsystem,
        EditorSubsystem
    >();

SE_BEGIN_REFLECT(EditorUISubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(EditorUISubsystem)

bool EditorUISubsystem::Initialize()
{
    const auto [window_subsystem, render_subsystem] = GetSubsystems<WindowSubsystem, const RenderSubsystem>();

    SDL_Window* main_window = window_subsystem->GetMainWindow();
    SDL_GPUDevice* raw_device = render_subsystem->GetRenderDevice().GetRawDevice();

    // ImGui 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    // EditorUI 설정 로드
    EditorUISettings ui_settings;
    if (auto result = ConfigFile::Load("Config://EditorConfig.toml"))
    {
        ui_settings = result.Value().GetSection<EditorUISettings>("ui");
    }

    // TODO: 나중에 다중모니터 지원하도록 변경
    const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    // 폰트 로드 (설정 파일에서 읽은 경로/크기 사용)
    if (const Optional ttf_path_opt = VFS::Resolve(VPath(ui_settings.font_path)))
    {
        io.Fonts->AddFontFromFileTTF(ttf_path_opt->ToString().CStr(), ui_settings.font_size, nullptr, io.Fonts->GetGlyphRangesKorean());
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "Failed to load font: {}", ui_settings.font_path);
    }

    ImGui_ImplSDL3_InitForSDLGPU(main_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
        .Device = raw_device,
        .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(raw_device, main_window),
        .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
    };
    ImGui_ImplSDLGPU3_Init(&init_info);

    // Platform Event 등록
    EventSubsystem& event_subsystem = GetSubsystemChecked<EventSubsystem>();
    sdl_event_handle = event_subsystem.on_sdl_event.AddLambda([](const SDL_Event& event)
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
    });

    // 일단 명시적으로 Register 코드 작성
    RegisterPanel<ImGuiDemoPanel>(GetTypeName<ImGuiDemoPanel>());
    RegisterPanel<DebugPanel>(GetTypeName<DebugPanel>());
    RegisterPanel<OutlinerPanel>(GetTypeName<OutlinerPanel>());
    RegisterPanel<DetailPanel>(GetTypeName<DetailPanel>());
    RegisterPanel<ViewportPanel>("ViewportPanel_Main", "ViewportPanel_Main", true);
    RegisterPanel<ViewportPanel>("ViewportPanel_Sub", "ViewportPanel_Sub", false);
    RegisterPanel<CameraPanel>(GetTypeName<CameraPanel>());
    RegisterPanel<AssetsBrowserPanel>(GetTypeName<AssetsBrowserPanel>());
    RegisterPanel<EditorConsolePanel>(GetTypeName<EditorConsolePanel>());
    RegisterPanel<SettingsPanel>(GetTypeName<SettingsPanel>());

    return true;
}

void EditorUISubsystem::Release()
{
    panels.Clear();

    // SDL 이벤트 구독 해제
    if (sdl_event_handle.IsValid())
    {
        if (EventSubsystem* event_subsystem = GetSubsystem<EventSubsystem>())
        {
            event_subsystem->on_sdl_event.Remove(sdl_event_handle);
        }
        sdl_event_handle.Invalidate();
    }

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

bool EditorUISubsystem::IsAnyPanelFocused() const
{
    return std::ranges::any_of(panels | std::views::values, [](const auto& panel)
    {
        return panel->IsFocused();
    });
}

Optional<const IEditorPanel&> EditorUISubsystem::GetPanel(const StringName& panel_id) const
{
    return panels.Find(panel_id)
        .AndThen([](const auto& panel_ptr) -> Optional<const IEditorPanel&>
        {
            return *panel_ptr;
        });
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void EditorUISubsystem::SetupDockSpace() // NOLINT(*-convert-member-functions-to-static)
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
                Application::Get().RequestQuit();
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
                if (const WorldSubsystem* world_subsystem = GetSubsystem<WorldSubsystem>())
                {
                    ecs::World* world = world_subsystem->GetWorld();
                    world->SpawnEntity();
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace se::editor
