#include "UI/Panels/SettingsPanel.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "imgui.h"


namespace
{
const se::VPath EngineConfigPath = "Config://EngineConfig.toml";
const se::VPath EditorConfigPath = "Config://EditorConfig.toml";
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

ImGuiWindowFlags SettingsPanel::GetWindowFlags() const
{
    return ImGuiWindowFlags_MenuBar;
}

void SettingsPanel::DrawContent()
{
    if (needs_reload)
    {
        LoadSettings();
        needs_reload = false;
    }

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
            struct CategoryEntry
            {
                ECategory value;
                String name;
            };

            static auto category_entries = []
            {
                FixedArray<CategoryEntry, EnumCount<ECategory>()> ret{};
                std::ranges::transform(EnumEntries<ECategory>(), ret.begin(), [](const auto& entry) -> CategoryEntry
                {
                    return {
                        .value = static_cast<ECategory>(entry.value),
                        .name = entry.name,
                    };
                });
                return ret;
            }();

            for (const auto& [value, name] : category_entries)
            {
                const bool is_selected = (current_category == value);
                if (ImGui::Selectable(name.CStr(), is_selected))
                {
                    current_category = value;
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
            {
                needs_save |= DrawSettings("Window", TypeId::Get<WindowSettings>(), &window_settings);
                ImGui::TextDisabled("(Window settings apply on next launch)");
                break;
            }
            case ECategory::UI:
            {
                needs_save |= DrawSettings("Editor UI", TypeId::Get<EditorUISettings>(), &ui_settings);
                ImGui::TextDisabled("(Font and theme changes apply on next launch)");
                break;
            }
            case ECategory::Console:
            {
                needs_save |= DrawSettings("Console", TypeId::Get<ConsoleSettings>(), &console_settings);
                break;
            }
            case ECategory::Performance:
            {
                needs_save |= DrawSettings("Performance", TypeId::Get<PerformanceSettings>(), &performance_settings);
                ImGui::TextDisabled("Target FPS applies immediately on save.");
                ImGui::TextDisabled("Busy wait ratio affects frame timing precision vs CPU usage.");
                break;
            }
            case ECategory::Graphics:
            {
                needs_save |= DrawSettings("Graphics", TypeId::Get<GraphicsSettings>(), &graphics_settings);
                ImGui::TextDisabled("(Present mode applies on next launch)");
                break;
            }
            default:
                SE_UNREACHABLE();
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
}

void SettingsPanel::LoadSettings()
{
    // Engine 설정 로드
    if (auto result = ConfigFile::Load(EngineConfigPath))
    {
        const ConfigFile& config = result.Value();
        window_settings = config.GetSection<WindowSettings>("window");
        performance_settings = config.GetSection<PerformanceSettings>("performance");
        graphics_settings = config.GetSection<GraphicsSettings>("graphics");
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "Engine config not found, using defaults. ({})", result.Error());
        window_settings = {};
        performance_settings = {};
        graphics_settings = {};
    }

    // Editor 설정 로드
    if (auto result = ConfigFile::Load(EditorConfigPath))
    {
        const ConfigFile& config = result.Value();
        ui_settings = config.GetSection<EditorUISettings>("ui");
        console_settings = config.GetSection<ConsoleSettings>("console");
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "Editor config not found, using defaults. ({})", result.Error());
        ui_settings = {};
        console_settings = {};
    }

    needs_save = false;
}

void SettingsPanel::SaveSettings()
{
    bool saved = true;

    // Engine 설정 저장
    {
        ConfigFile config;
        if (auto result = ConfigFile::Load(EngineConfigPath))
        {
            config = std::move(result).Value();
        }

        config.SetSection(window_settings, "window");
        config.SetSection(performance_settings, "performance");
        config.SetSection(graphics_settings, "graphics");

        if (!config.Save(EngineConfigPath))
        {
            ConsoleLog(ELogLevel::Error, "Failed to save engine settings to {}", EngineConfigPath.ToString());
            saved = false;
        }
    }

    // Editor 설정 저장
    {
        ConfigFile config;
        if (auto result = ConfigFile::Load(EditorConfigPath))
        {
            config = std::move(result).Value();
        }

        config.SetSection(ui_settings, "ui");
        config.SetSection(console_settings, "console");

        if (!config.Save(EditorConfigPath))
        {
            ConsoleLog(ELogLevel::Error, "Failed to save editor settings to {}", EditorConfigPath.ToString());
            saved = false;
        }
    }

    if (saved)
    {
        ConsoleLog(ELogLevel::Info, "Settings saved.");
        needs_save = false;

        // 즉시 적용 가능한 설정 반영
        Application::SetTargetFps(performance_settings.target_fps);
        Application::SetBusyWaitRatio(performance_settings.busy_wait_ratio);
    }
}

bool SettingsPanel::DrawSettings(const char* label, const TypeId& type_id, void* settings_ptr)
{
    ImGui::SeparatorText(label);

    const TypeInfo& settings = TypeRegistry::Get().FindChecked(type_id);
    return DrawerRegistry::Get().DrawProperties(settings, settings_ptr);
}
} // namespace se::editor
