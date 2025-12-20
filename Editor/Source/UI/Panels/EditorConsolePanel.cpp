#include "UI/Panels/EditorConsolePanel.h"

#include "Core/Logging/Backend/EditorConsoleBackend.h"
#include "SimpleEngine/Core/Logging/LogBackendManager.h"

#include "imgui.h"
#include "SimpleEngine/Core/Logging/Logging.h"


namespace
{
[[nodiscard]] ImVec4 GetColorForLevel(se::ELogLevel level)
{
    switch (level)
    {
    case se::ELogLevel::Debug:
        // 회색 (Gray) - 덜 중요함
        return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

    case se::ELogLevel::Info:
        // 흰색 (White) - 기본
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    case se::ELogLevel::Warning:
        // 노란색 (Yellow)
        return ImVec4(1.0f, 0.9f, 0.2f, 1.0f);

    case se::ELogLevel::Error:
        // 밝은 빨간색 (Light Red)
        return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

    case se::ELogLevel::Fatal:
        // 자주색/보라색 (Magenta/Purple) - 치명적임
        return ImVec4(1.0f, 0.0f, 1.0f, 1.0f);

    default:
        // 혹시 모를 경우 기본 텍스트 색상
        return ImGui::GetStyle().Colors[ImGuiCol_Text];
    }
}
}

namespace se::editor::ui
{
const char* EditorConsolePanel::GetName() const
{
    return "Console";
}

void EditorConsolePanel::Draw()
{
    static EditorConsoleBackend* backend = se::core::LogBackendManager::Get().GetBackend<EditorConsoleBackend>();
    if (!backend)
    {
        ConsoleLog(ELogLevel::Error, "Failed to get editor console backend!");
        SetVisibility(false);
        return;
    }

    ImGui::Begin(GetName(), &is_visible);
    {
        // 상단 제어 버튼
        if (ImGui::Button("Clear"))
        {
            backend->Clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        // ... 필터 등 추가 ...

        ImGui::Separator();

        // 스크롤 영역 시작
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            backend->ReadLogs([this](const Deque<se::core::LogEntry>& logs)
            {
                ImGuiListClipper clipper;

                // Clipper에게 전체 아이템 개수를 알려줌
                clipper.Begin(static_cast<int>(logs.Len()));

                // Clipper가 지정한 범위(화면에 보이는 범위)만 렌더링
                while (clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                    {
                        const auto& log = logs[i]; // Deque의 Random Access

                        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(log.level));
                        ImGui::Text(
                            "%-7s [%s:%d] %s",
                            log.GetLevelString(),
                            log.GetPrettyFileName().data(),
                            log.location.line(),
                            log.formatted_message.CStr()
                        );
                        ImGui::PopStyleColor();
                    }
                }

                // Auto Scroll 처리
                if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                {
                    ImGui::SetScrollHereY(1.0f);
                }
            });
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
}
