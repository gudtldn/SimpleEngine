#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/View/ViewSettings.h"


namespace se::editor
{
/**
 * 3D 씬을 렌더링하는 뷰포트 패널
 */
class ViewportPanel : public IEditorPanel
{
public:
    explicit ViewportPanel(const StringName& in_viewport_id, bool default_visibility);

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

private:
    /** 뷰포트 상단에 반투명 오버레이 툴바를 렌더링합니다. */
    void DrawToolbar(const ImVec2& content_min, const ImVec2& content_size);

    StringName viewport_id;

    // 렌더 설정
    ERenderingMode rendering_mode = ERenderingMode::Lit;
    ShowFlags show_flags = EShowFlag::Grid | EShowFlag::StaticMesh;
};
} // namespace se::editor
