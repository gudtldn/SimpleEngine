#include "UI/Panels/SettingsPanel.h"

#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "UI/ImGui/ImGuiString.h"
#include "imgui.h"


namespace
{
const se::VPath ConfigPath = "Config://EngineConfig.toml";

const char* category_names[] = {
    "Window",
    "UI",
    "Console",
    "Asset Browser",
    "Performance",
    "Graphics",
};

static_assert(std::size(category_names) == static_cast<size_t>(se::editor::SettingsPanel::ECategory::COUNT));
} // namespace

namespace se::editor
{
SettingsPanel::SettingsPanel()
{
    is_visible = false;  // 기본적으로 숨김 상태로 시작
}

const char* SettingsPanel::GetName() const
{
    return "Settings";
}

void SettingsPanel::Draw()
{
    if (needs_reload)
    {
        LoadSettings();
        needs_reload = false;
    }

    ImGui::Begin(GetName(), &is_visible, ImGuiWindowFlags_MenuBar);

    // 메뉴 바
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::MenuItem("Save"))
        {
            SaveSettings();
        }
        if (ImGui::MenuItem("Reload"))
        {
            needs_reload = true;
        }
        ImGui::EndMenuBar();
    }

    // 2열 레이아웃: 카테고리 목록 | 설정 편집기
    if (ImGui::BeginTable("SettingsLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("Categories", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch);

        // 좌측: 카테고리 목록
        ImGui::TableNextColumn();
        ImGui::BeginChild("CategoryList");
        {
            for (int i = 0; i < static_cast<int32>(ECategory::COUNT); ++i)
            {
                const bool is_selected = (current_category == static_cast<ECategory>(i));
                if (ImGui::Selectable(category_names[i], is_selected))
                {
                    current_category = static_cast<ECategory>(i);
                }
            }
        }
        ImGui::EndChild();

        // 우측: 설정 편집기
        ImGui::TableNextColumn();
        ImGui::BeginChild("SettingsEditor");
        {
            switch (current_category)
            {
            case ECategory::Window:
                DrawWindowSettings();
                break;
            case ECategory::UI:
                DrawUISettings();
                break;
            case ECategory::Console:
                DrawConsoleSettings();
                break;
            case ECategory::AssetBrowser:
                DrawAssetBrowserSettings();
                break;
            case ECategory::Performance:
                DrawPerformanceSettings();
                break;
            case ECategory::Graphics:
                DrawGraphicsSettings();
                break;
            default: break;
            }
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    // 하단 저장 안내
    if (needs_save)
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes. Click Save or Ctrl+S to apply.");
    }

    ImGui::End();
}

void SettingsPanel::LoadSettings()
{
    auto result = ConfigFile::Load(ConfigPath);
    if (!result.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "SettingsPanel: Config file not found, using defaults. ({})", result.Error());
        // 기본값 구조체 그대로 사용
        window_settings = {};
        ui_settings = {};
        console_settings = {};
        asset_browser_settings = {};
        performance_settings = {};
        graphics_settings = {};
        return;
    }

    ConfigFile& config = result.Value();
    window_settings = config.GetSection<WindowSettings>("window");
    ui_settings = config.GetSection<EditorUISettings>("editor.ui");
    console_settings = config.GetSection<ConsoleSettings>("editor.console");
    asset_browser_settings = config.GetSection<AssetBrowserSettings>("editor.asset_browser");
    performance_settings = config.GetSection<PerformanceSettings>("performance");
    graphics_settings = config.GetSection<GraphicsSettings>("graphics");

    needs_save = false;
}

void SettingsPanel::SaveSettings()
{
    // 기존 파일을 로드 (다른 섹션 보존)
    ConfigFile config;
    if (auto result = ConfigFile::Load(ConfigPath))
    {
        config = std::move(result).Value();
    }

    config.SetSection(window_settings, "window");
    config.SetSection(ui_settings, "editor.ui");
    config.SetSection(console_settings, "editor.console");
    config.SetSection(asset_browser_settings, "editor.asset_browser");
    config.SetSection(performance_settings, "performance");
    config.SetSection(graphics_settings, "graphics");

    if (config.Save(ConfigPath))
    {
        ConsoleLog(ELogLevel::Info, "Settings saved to {}", ConfigPath.ToString());
        needs_save = false;

        // 즉시 적용 가능한 설정 반영
        Application::SetTargetFps(performance_settings.target_fps);
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "Failed to save settings to {}", ConfigPath.ToString());
    }
}

// ============================================================================
//  카테고리별 UI
// ============================================================================

void SettingsPanel::DrawWindowSettings()
{
    ImGui::SeparatorText("Window");

    // Title
    if (ImGui::InputText("Title", &window_settings.title))
    {
        needs_save = true;
    }

    // Width / Height
    int w = static_cast<int>(window_settings.width);
    int h = static_cast<int>(window_settings.height);
    if (ImGui::DragInt("Width", &w, 1.0f, 320, 7680))
    {
        window_settings.width = static_cast<uint32>(w);
        needs_save = true;
    }
    if (ImGui::DragInt("Height", &h, 1.0f, 240, 4320))
    {
        window_settings.height = static_cast<uint32>(h);
        needs_save = true;
    }

    // Flags
    needs_save |= ImGui::Checkbox("Fullscreen", &window_settings.fullscreen);
    needs_save |= ImGui::Checkbox("Borderless", &window_settings.borderless);
    needs_save |= ImGui::Checkbox("Resizable", &window_settings.resizable);

    ImGui::TextDisabled("(Window settings apply on next launch)");
}

void SettingsPanel::DrawUISettings()
{
    ImGui::SeparatorText("Editor UI");

    // Font Path
    if (ImGui::InputText("Font Path", &ui_settings.font_path))
    {
        needs_save = true;
    }

    // Font Size
    if (ImGui::DragFloat("Font Size", &ui_settings.font_size, 0.5f, 8.0f, 48.0f, "%.1f"))
    {
        needs_save = true;
    }

    // Theme
    const char* themes[] = { "dark", "light", "classic" };
    int current_theme = 0;
    if (ui_settings.theme == "light")
    {
        current_theme = 1;
    }
    else if (ui_settings.theme == "classic")
    {
        current_theme = 2;
    }

    if (ImGui::Combo("Theme", &current_theme, themes, 3))
    {
        ui_settings.theme = themes[current_theme];
        needs_save = true;
    }

    ImGui::TextDisabled("(Font and theme changes apply on next launch)");
}

void SettingsPanel::DrawConsoleSettings()
{
    ImGui::SeparatorText("Console");

    needs_save |= ImGui::Checkbox("Auto Scroll", &console_settings.auto_scroll);
    needs_save |= ImGui::Checkbox("Show Timestamp", &console_settings.show_timestamp);
    needs_save |= ImGui::Checkbox("Show Thread Name", &console_settings.show_thread_name);
    needs_save |= ImGui::Checkbox("Show Location", &console_settings.show_location);

    int max_lines = static_cast<int>(console_settings.max_log_lines);
    if (ImGui::DragInt("Max Log Lines", &max_lines, 10.0f, 100, 50000))
    {
        console_settings.max_log_lines = static_cast<uint32>(max_lines);
        needs_save = true;
    }
}

void SettingsPanel::DrawAssetBrowserSettings()
{
    ImGui::SeparatorText("Asset Browser");

    needs_save |= ImGui::DragFloat("Grid Padding", &asset_browser_settings.grid_padding, 0.5f, 0.0f, 64.0f, "%.1f");
    needs_save |= ImGui::DragFloat("Thumbnail Size", &asset_browser_settings.thumbnail_size, 1.0f, 32.0f, 256.0f, "%.0f");
    needs_save |= ImGui::DragFloat("Tree Column Width", &asset_browser_settings.tree_column_width, 1.0f, 80.0f, 400.0f, "%.0f");
}

void SettingsPanel::DrawPerformanceSettings()
{
    ImGui::SeparatorText("Performance");

    int fps = static_cast<int>(performance_settings.target_fps);
    if (ImGui::DragInt("Target FPS", &fps, 1.0f, 1, 2400))
    {
        performance_settings.target_fps = static_cast<uint32>(fps);
        needs_save = true;
    }

    if (ImGui::DragFloat("Busy Wait Ratio", &performance_settings.busy_wait_ratio, 0.01f, 0.0f, 1.0f, "%.2f"))
    {
        needs_save = true;
    }

    ImGui::TextDisabled("Target FPS applies immediately on save.");
    ImGui::TextDisabled("Busy wait ratio affects frame timing precision vs CPU usage.");
}

void SettingsPanel::DrawGraphicsSettings()
{
    ImGui::SeparatorText("Graphics");

    // Present Mode
    const char* modes[] = { "mailbox", "vsync", "immediate" };
    int current_mode = 0;
    if (graphics_settings.present_mode == "vsync")
    {
        current_mode = 1;
    }
    else if (graphics_settings.present_mode == "immediate")
    {
        current_mode = 2;
    }

    if (ImGui::Combo("Present Mode", &current_mode, modes, 3))
    {
        graphics_settings.present_mode = modes[current_mode];
        needs_save = true;
    }

    // Clear Color
    ImGui::Text("Scene Clear Color");
    float clear_color[3] = {
        graphics_settings.clear_color_r,
        graphics_settings.clear_color_g,
        graphics_settings.clear_color_b,
    };
    if (ImGui::ColorEdit3("Clear Color", clear_color))
    {
        graphics_settings.clear_color_r = clear_color[0];
        graphics_settings.clear_color_g = clear_color[1];
        graphics_settings.clear_color_b = clear_color[2];
        needs_save = true;
    }

    ImGui::TextDisabled("(Present mode applies on next launch)");
}
}  // namespace se::editor
