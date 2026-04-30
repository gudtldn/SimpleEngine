#include "UI/Panels/EditorConsolePanel.h"

#include "Core/Logging/Backend/EditorConsoleBackend.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Logging/LogBackendManager.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Utility/Debug.h"

#include "imgui.h"


namespace
{
[[nodiscard]] ImVec4 GetColorForLevel(se::ELogLevel level)
{
    switch (level)
    {
    case se::ELogLevel::Debug:
        // Gray
        return { 0.6f, 0.6f, 0.6f, 1.0f };

    case se::ELogLevel::Info:
        // White
        return { 1.0f, 1.0f, 1.0f, 1.0f };

    case se::ELogLevel::Warning:
        // Yellow
        return { 1.0f, 0.9f, 0.2f, 1.0f };

    case se::ELogLevel::Error:
        // Red
        return { 1.0f, 0.3f, 0.3f, 1.0f };

    case se::ELogLevel::Fatal:
        // Magenta
        return { 1.0f, 0.0f, 1.0f, 1.0f };

    default:
        // 기본 텍스트 색상
        return ImGui::GetStyle().Colors[ImGuiCol_Text];
    }
}
} // namespace

namespace se::editor
{
EditorConsolePanel::EditorConsolePanel()
{
    LoadSettings();
}

const char* EditorConsolePanel::GetName() const
{
    return "Console";
}

void EditorConsolePanel::DrawContent()
{
    EditorConsoleBackend* backend = se::LogBackendManager::Get().GetBackend<EditorConsoleBackend>();
    if (!backend)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get editor console backend!");
        SetVisibility(false);
        return;
    }

    // 상단 제어 버튼
    if (ImGui::Button("Clear"))
    {
        backend->Clear();
        filter_changed = true;
    }

    ImGui::SameLine();

    // Options Dropdown
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("OptionsPopup");
    }

    if (ImGui::BeginPopup("OptionsPopup"))
    {
        bool options_changed = false;
        options_changed |= ImGui::Checkbox("Auto-scroll", &settings.auto_scroll);
        ImGui::Separator();
        options_changed |= ImGui::Checkbox("Show Timestamp", &settings.show_timestamp);
        options_changed |= ImGui::Checkbox("Show Thread Name", &settings.show_thread_name);
        options_changed |= ImGui::Checkbox("Show Location", &settings.show_location);

        if (options_changed)
        {
            SaveSettings();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Level Filters Dropdown
    if (ImGui::Button("Levels"))
    {
        ImGui::OpenPopup("LevelsPopup");
    }

    if (ImGui::BeginPopup("LevelsPopup"))
    {
        filter_changed |= ImGui::Checkbox("Debug", &filter_debug);
        filter_changed |= ImGui::Checkbox("Info", &filter_info);
        filter_changed |= ImGui::Checkbox("Warning", &filter_warning);
        filter_changed |= ImGui::Checkbox("Error", &filter_error);
        filter_changed |= ImGui::Checkbox("Fatal", &filter_fatal);
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Text Filter
    ImGui::SetNextItemWidth(-150.0f); // 우측 여백을 조금 남김
    filter_changed |= text_filter.Draw("Search ###ConsoleFilter");

    ImGui::Separator();

    // 스크롤 영역 시작
    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
    {
        backend->ReadLogs([this](const Deque<se::LogEntry>& entries)
        {
            if (entries.Len() != last_log_count)
            {
                filter_changed = true;
            }

            if (filter_changed)
            {
                RefreshFilterList(entries);
            }

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(cached_indices.Len())); // Clipper에게 전체 아이템 개수를 알려줌

            // Clipper가 지정한 범위(화면에 보이는 범위)만 렌더링
            while (clipper.Step())
            {
                for (int idx = clipper.DisplayStart; idx < clipper.DisplayEnd; ++idx)
                {
                    const usize log_idx = cached_indices[idx];
                    if (log_idx >= entries.Len())
                    {
                        continue;
                    }

                    const auto& entry = entries[log_idx];

                    // Rendering Timestamp (Gray)
                    if (settings.show_timestamp)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::TextUnformatted(entry.GetTimestampString().c_str());
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                    }

                    // Rendering Thread Name (Cyan)
                    if (settings.show_thread_name)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.8f, 0.8f, 1.0f));
                        ImGui::Text("[%s]", entry.thread_name.CStr());
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                    }

                    // Rendering Log Level (Color based on level)
                    ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(entry.level));
                    ImGui::TextUnformatted(entry.GetLevelString());
                    ImGui::PopStyleColor();
                    ImGui::SameLine();

                    // Rendering File Location (Dimmed White)
                    if (settings.show_location)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                        const StringView pretty_name = entry.GetPrettyFileName();
                        ImGui::Text("[%.*s:%d]",
                            static_cast<int>(pretty_name.ByteLen()),
                            pretty_name.Data(),
                            entry.location.line()
                        );
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                    }

                    // Rendering Message
                    ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(entry.level));
                    ImGui::Text("%s", entry.formatted_message.CStr());
                    ImGui::PopStyleColor();
                }
            }
        });

        // Auto Scroll 처리
        if (settings.auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
}

void EditorConsolePanel::LoadSettings()
{
    const VPath config_path = "Config://EditorConfig.toml";
    if (auto result = ConfigFile::Load(config_path))
    {
        settings = result.Value().GetSection<ConsoleSettings>("console");
    }

    // Backend에 max_log_lines 반영
    if (EditorConsoleBackend* backend = LogBackendManager::Get().GetBackend<EditorConsoleBackend>())
    {
        backend->SetMaxLogLines(settings.max_log_lines);
    }
}

void EditorConsolePanel::SaveSettings()
{
    const VPath config_path = "Config://EditorConfig.toml";
    ConfigFile config;
    if (auto result = ConfigFile::Load(config_path))
    {
        config = std::move(result).Value();
    }
    config.SetSection(settings, "console");
    std::ignore = config.Save(config_path);
}

void EditorConsolePanel::RefreshFilterList(const Deque<se::LogEntry>& logs)
{
    cached_indices.Clear();
    cached_indices.Reserve(logs.Len());

    for (auto [idx, entry] : logs | std::views::enumerate)
    {
        // Level Filtering
        bool level_pass = false;
        switch (entry.level)
        {
        case ELogLevel::Debug:
        {
            level_pass = filter_debug;
            break;
        }
        case ELogLevel::Info:
        {
            level_pass = filter_info;
            break;
        }
        case ELogLevel::Warning:
        {
            level_pass = filter_warning;
            break;
        }
        case ELogLevel::Error:
        {
            level_pass = filter_error;
            break;
        }
        case ELogLevel::Fatal:
        {
            level_pass = filter_fatal;
            break;
        }
        default:
            SE_UNREACHABLE();
        }

        if (!level_pass)
        {
            continue;
        }

        // Text Filtering
        if (!text_filter.PassFilter(entry.formatted_message.CStr()))
        {
            continue;
        }

        cached_indices.Push(idx);
    }

    filter_changed = false;
    last_log_count = logs.Len();
}
} // namespace se::editor
