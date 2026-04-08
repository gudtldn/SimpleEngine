#pragma once

#include "SimpleEditor/EditorAPI.h"
#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
/**
 * 기즈모 렌더링 리소스를 소유하고 관리하는 서브시스템
 * GizmoDrawList의 GPU 버퍼 생명주기를 관리합니다.
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) GizmoSubsystem : public SubsystemBase
{
    SE_CLASS(GizmoSubsystem, SubsystemBase)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    [[nodiscard]] GizmoDrawList& GetDrawList() { return *draw_list; }
    [[nodiscard]] const GizmoDrawList& GetDrawList() const { return *draw_list; }

private:
    std::unique_ptr<GizmoDrawList> draw_list;
};
} // namespace se::editor
