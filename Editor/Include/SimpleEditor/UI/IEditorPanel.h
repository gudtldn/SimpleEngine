#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "imgui.h"


namespace se::editor
{
/**
 * Editor Panel의 기본 인터페이스
 */
class SE_EDITOR_API IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    /** 패널의 이름을 반환합니다. (ImGui Window 제목으로 사용) */
    [[nodiscard]] virtual const char* GetName() const = 0;

    /**
     * 패널을 렌더링합니다.
     * @note Begin/End 및 포커스, 호버 상태 갱신을 처리합니다.
     *       특수한 렌더링이 필요한 경우 오버라이드해서 사용하세요.
     */
    virtual void Draw();

    /** 현재 패널이 화면에 표시되는 상태인지 확인합니다. */
    [[nodiscard]] bool IsVisible() const { return is_visible; }

    /** 패널의 표시 여부를 설정하여 열거나 닫습니다. */
    void SetVisibility(bool new_visibility) { is_visible = new_visibility; }

    /** 현재 패널 윈도우가 ImGui 포커스 상태인지 확인합니다. */
    [[nodiscard]] bool IsFocused() const { return is_focused; }

    /** 현재 패널 윈도우에 마우스가 올라가 있는지 확인합니다. */
    [[nodiscard]] bool IsHovered() const { return is_hovered; }

protected:
    /** ImGui 윈도우 생성 시 사용할 플래그를 반환합니다. */
    [[nodiscard]] virtual ImGuiWindowFlags GetWindowFlags() const { return 0; }

    /** 패널 콘텐츠를 렌더링합니다. ImGui Begin/End 없이 구현합니다. */
    virtual void DrawContent() {}

protected:
    bool is_visible = true;
    bool is_focused = false;
    bool is_hovered = false;
};
}  // namespace se::editor
