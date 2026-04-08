#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEditor/Gizmo/GizmoRenderer.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
/**
 * 기즈모 렌더링 리소스를 소유하고 관리하는 Subsystem
 * GizmoDrawList(GPU 버퍼) + GizmoRenderer(형상 조립)의 생명주기를 관리합니다.
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) GizmoSubsystem : public SubsystemBase
{
    SE_CLASS(GizmoSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    /** 매 프레임 정점을 수집/업로드하는 드로우 리스트 접근자 */
    [[nodiscard]] GizmoDrawList& GetDrawList() { return *draw_list; }
    [[nodiscard]] const GizmoDrawList& GetDrawList() const { return *draw_list; }

    /** 기즈모 형상 조립(Translate/Rotate/Scale) 렌더러 접근자 */
    [[nodiscard]] GizmoRenderer& GetRenderer() { return renderer; }
    [[nodiscard]] const GizmoRenderer& GetRenderer() const { return renderer; }

private:
    std::unique_ptr<GizmoDrawList> draw_list;
    GizmoRenderer renderer;
};
} // namespace se::editor
