#pragma once

#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "UI/Panels/IEditorPanel.h"

#include "imgui.h"
#include "SimpleEngine/Core/Container/Array.h"


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
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    void RefreshFilterList(const Deque<se::LogEntry>& logs);

private:
    // TODO: EngineConfig.toml에 기록
    // --- Settings ---
    bool auto_scroll = true;
    bool show_timestamp = false;
    bool show_thread_name = false;
    bool show_location = true;

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
}  // namespace se::editor
