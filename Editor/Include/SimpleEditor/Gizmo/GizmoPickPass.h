#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
class GizmoDrawList;

/**
 * GPU Color Picking 전용 기즈모 렌더 패스
 *
 * Pick Matrix를 적용하여 커서 위치의 단일 픽셀만 1x1 R32_UINT 텍스처에 래스터라이즈합니다.
 * 출력 값은 GizmoVertex::pick_id (= EGizmoAxis underlying value) 입니다.
 */
class SE_EDITOR_API GizmoPickPass : public se::graphics::RenderPassBase
{
    SE_CLASS(GizmoPickPass, se::graphics::RenderPassBase)

public:
    /**
     * @param in_draw_list 이번 프레임에 수집된 기즈모 정점 데이터
     * @param in_render_view VP 행렬을 포함한 렌더 뷰
     * @param in_pick_target 1x1 R32_UINT 텍스처 핸들 (ImportTexture로 생성)
     * @param in_cursor_pos 뷰포트 로컬 커서 (X, Y)좌표
     */
    GizmoPickPass(
        const GizmoDrawList& in_draw_list,
        const graphics::RenderView& in_render_view,
        graphics::RGTextureHandle in_pick_target,
        Vector2f in_cursor_pos
    );

    virtual void Setup(graphics::RGSetupContext& context) override;
    virtual void Execute(graphics::RGExecutionContext& context) override;

private:
    const GizmoDrawList& draw_list;
    graphics::RenderView render_view;
    graphics::RGTextureHandle pick_target_handle;
    Vector2f cursor_pos;
};
} // namespace se::editor
