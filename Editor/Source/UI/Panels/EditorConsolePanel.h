#pragma once

#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "imgui.h"


namespace se
{
struct LogEntry;
}

namespace se::editor
{
/**
 * 엔진의 로그를 출력하는 에디터 콘솔 패널
 */
class EditorConsolePanel : public IEditorPanel
{
public:
    EditorConsolePanel();

    [[nodiscard]] virtual const char* GetName() const override;

protected:
    virtual void DrawContent() override;

private:
    void LoadSettings();
    void SaveSettings();
    void RefreshFilterList(const Deque<LogEntry>& logs);

private:
    ConsoleSettings settings;

    // --- Filters ---
    bool filter_debug = true;
    bool filter_info = true;
    bool filter_warning = true;
    bool filter_error = true;
    bool filter_fatal = true;

    // Text Filter
    ImGuiTextFilter text_filter;

    // 필터링된 로그의 인덱스들을 저장해두는 캐시
    Array<usize> cached_indices;

    // 필터 조건이 변경되었는지 추적하는 플래그
    bool filter_changed = false;
    usize last_log_count = 0;
};
} // namespace se::editor
