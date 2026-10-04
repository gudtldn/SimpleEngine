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
#include "Panels/WorldResourcePanel.h"
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/Core/SelectionSubsystem.h"

#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/HAL/FileDialog.h"
#include "SimpleEngine/Core/HAL/WindowSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/JsonArchive.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/ECS/EntitySubsystem.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/WorldFile.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
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
        SelectionSubsystem
    >()
    .UpdateDependsOn<EntitySubsystem>();

SE_BEGIN_REFLECT_V1(EditorUISubsystem, meta::Reflect, meta::Hidden, meta::Transient)
    SE_REFLECT_INTERFACE_V1(IUpdatable)
SE_END_REFLECT_V1(EditorUISubsystem)

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
        ui_settings = result->GetSection<EditorUISettings>("ui");
    }

    // TODO: 나중에 다중모니터 지원하도록 변경
    const f32 main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

    // 폰트 로드 (설정 파일에서 읽은 경로/크기 사용)
    if (const auto ttf_path = VFS::Resolve(VPath(ui_settings.font_path)))
    {
        io.Fonts->AddFontFromFileTTF(ttf_path->CStr(), ui_settings.font_size, nullptr, io.Fonts->GetGlyphRangesKorean());
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
    RegisterPanel<AssetsBrowserPanel>(GetTypeName<AssetsBrowserPanel>());
    RegisterPanel<CameraPanel>(GetTypeName<CameraPanel>());
    RegisterPanel<DebugPanel>(GetTypeName<DebugPanel>());
    RegisterPanel<DetailPanel>(GetTypeName<DetailPanel>());
    RegisterPanel<EditorConsolePanel>(GetTypeName<EditorConsolePanel>());
    RegisterPanel<ImGuiDemoPanel>(GetTypeName<ImGuiDemoPanel>());
    RegisterPanel<OutlinerPanel>(GetTypeName<OutlinerPanel>());
    RegisterPanel<SettingsPanel>(GetTypeName<SettingsPanel>());
    RegisterPanel<ViewportPanel>("ViewportPanel_Main", "ViewportPanel_Main", true);
    RegisterPanel<ViewportPanel>("ViewportPanel_Sub1", "ViewportPanel_Sub1", false);
    RegisterPanel<ViewportPanel>("ViewportPanel_Sub2", "ViewportPanel_Sub2", false);
    RegisterPanel<ViewportPanel>("ViewportPanel_Sub3", "ViewportPanel_Sub3", false);
    RegisterPanel<WorldResourcePanel>(GetTypeName<WorldResourcePanel>());

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

void EditorUISubsystem::Update([[maybe_unused]] f64 delta_time)
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
    static constexpr FileFilter WORLD_FILTER = {
        .name = "SimpleEngine World",
        .pattern = "seworld"
    };

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New World", "Ctrl+N"))
            {
                if (EntitySubsystem* entity_sub = GetSubsystem<EntitySubsystem>())
                {
                    World& world = entity_sub->GetMainWorld().GetWorld();
                    world.Reset();
                    ConsoleLog(ELogLevel::Info, "World reset.");
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Save World", "Ctrl+S"))
            {
                if (EntitySubsystem* entity_sub = GetSubsystem<EntitySubsystem>())
                {
                    // 다이얼로그 표시 전에 직렬화하여 현재 상태를 캡처
                    WorldFile world_file{ entity_sub->GetMainWorld().GetWorld() };
                    JsonWriter writer;
                    const auto save_result = world_file.Save(writer);
                    for (const String& warning : world_file.GetWarnings())
                    {
                        ConsoleLog(ELogLevel::Warning, "{}", warning);
                    }

                    if (save_result.HasError())
                    {
                        ConsoleLog(ELogLevel::Error, "Failed to save world: {}", save_result.Error());
                    }
                    else if (auto content = writer.ToText(); content.HasError())
                    {
                        ConsoleLog(ELogLevel::Error, "Failed to save world: {}", content.Error());
                    }
                    else
                    {
                        FileDialog::SaveFile(
                            [con = std::move(content).Value()](const Path& path)
                            {
                                if (fs::WriteString(path, con))
                                {
                                    ConsoleLog(ELogLevel::Info, "World saved: {}", path);
                                }
                                else
                                {
                                    ConsoleLog(ELogLevel::Error, "Failed to save world: {}", path);
                                }
                            },
                            { WORLD_FILTER }
                        );
                    }
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Load World", "Ctrl+O"))
            {
                FileDialog::OpenFile(
                    [](const Path& path)
                    {
                        // 파일 읽기는 다이얼로그 스레드에서 수행 (I/O만)
                        FileResult<Array<u8>> bytes_result = fs::ReadBytes(path);
                        if (bytes_result.HasError())
                        {
                            ConsoleLog(ELogLevel::Error, "Failed to read file: {}, Err: {}", path, bytes_result.Error().What());
                            return;
                        }

                        // World 수정은 메인 스레드에서 수행
                        JobSystem::Get().DispatchToMain([data = std::move(bytes_result).Value(), file_path = path]
                        {
                            EntitySubsystem* entity_sub = GetSubsystem<EntitySubsystem>();
                            if (!entity_sub)
                            {
                                return;
                            }

                            World& world = entity_sub->GetMainWorld().GetWorld();

                            // 읽기에 성공했을 때만 기존 월드를 비우고 파일의 영속 ID 그대로 읽음 (Resource는 유지)
                            JsonReader reader{ StringView{ reinterpret_cast<const char*>(data.Data()), data.Len() } };
                            WorldFile world_file{ world };
                            const auto result = world_file.Replace(reader);
                            for (const String& warning : world_file.GetWarnings())
                            {
                                ConsoleLog(ELogLevel::Warning, "{}", warning);
                            }
                            if (result.HasError())
                            {
                                ConsoleLog(ELogLevel::Error, "Failed to load world: {}", result.Error());
                                return;
                            }

                            ConsoleLog(ELogLevel::Info, "World loaded: {}", file_path);
                        });
                    },
                    { WORLD_FILTER }
                );
            }

            ImGui::Separator();

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
                if (EntitySubsystem* entity_subsystem = GetSubsystem<EntitySubsystem>())
                {
                    entity_subsystem->GetMainWorld().GetWorld().SpawnEntity();
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace se::editor
