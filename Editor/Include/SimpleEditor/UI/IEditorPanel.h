#pragma once
#include <editor_export.h>


namespace se::editor
{
/**
 * Editor Panel의 기본 인터페이스
 */
class SE_EDITOR_API IEditorPanel
{
public:
    virtual ~IEditorPanel() = default;

    // 패널의 이름을 반환합니다 (ImGui Window 제목으로 사용)
    [[nodiscard]] virtual const char* GetName() const = 0;

    // 실제 ImGui 코드가 들어갈 함수
    virtual void Draw() = 0;

    // 패널이 열려있는지 여부를 관리
    [[nodiscard]] bool IsVisible() const { return is_visible; }
    void SetVisibility(bool new_visibility) { is_visible = new_visibility; }

protected:
    bool is_visible = true;
};
}  // namespace se::editor
