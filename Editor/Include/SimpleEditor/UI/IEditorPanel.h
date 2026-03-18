#pragma once

#include "SimpleEditor/EditorCommon.h"


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

    /** 패널의 UI를 렌더링합니다. */
    virtual void Draw() = 0;

    /** 현재 패널이 화면에 표시되는 상태인지 확인합니다. */
    [[nodiscard]] bool IsVisible() const { return is_visible; }

    /** 패널의 표시 여부를 설정하여 열거나 닫습니다. */
    void SetVisibility(bool new_visibility) { is_visible = new_visibility; }

protected:
    bool is_visible = true;
};
}  // namespace se::editor
