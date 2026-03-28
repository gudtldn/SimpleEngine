#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se::editor
{
/** 기즈모 조작 모드 */
enum class EGizmoMode : uint8
{
    Translate,
    Rotate,
    Scale,
};

/** 좌표계 모드 */
enum class ECoordinateSpace : uint8
{
    World,
    Local,
};

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

    // 툴바 상태
    EGizmoMode gizmo_mode = EGizmoMode::Translate;
    ECoordinateSpace coordinate_space = ECoordinateSpace::World;
};
} // namespace se::editor
