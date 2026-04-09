#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::editor
{
class GizmoDrawList;

/** 기즈모를 렌더링하는 패스 */
class SE_EDITOR_API GizmoPass : public se::graphics::RenderPassBase
{
    SE_CLASS(GizmoPass, se::graphics::RenderPassBase)

public:
    /**
     * @param in_draw_list 이번 프레임에 수집된 기즈모 정점 데이터
     * @param in_render_view VP 행렬을 포함한 렌더 뷰
     * @param in_color_target 렌더 그래프 컬러 텍스처 핸들
     * @param in_depth_target 렌더 그래프 뎁스 텍스처 핸들 (읽기 전용, depth test OFF)
     */
    GizmoPass(
        const GizmoDrawList& in_draw_list,
        const graphics::RenderView& in_render_view,
        graphics::RGTextureHandle in_color_target,
        graphics::RGTextureHandle in_depth_target
    );

    /** 렌더 그래프에 커러/뎁스 타겟을 등록하고, 파이프라인 2개(LINELIST + TRIANGLELIST)를 생성합니다 */
    virtual void Setup(graphics::RGSetupContext& context) override;

    /** 2회 드로우(LINELIST + TRIANGLELIST)를 발행하여 기즈모를 렌더링합니다 */
    virtual void Execute(graphics::RGExecutionContext& context) override;

private:
    const GizmoDrawList& draw_list;
    graphics::RenderView render_view;
    graphics::RGTextureHandle color_target_handle;
    graphics::RGTextureHandle depth_target_handle;
};
} // namespace se::editor
