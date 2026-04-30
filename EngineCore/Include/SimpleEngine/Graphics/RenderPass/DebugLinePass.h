#pragma once

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


// forward declaration
namespace se
{
class DebugDrawSubsystem;
}

namespace se
{
/**
 * DebugLine을 렌더링하는 패스
 *
 * - Depth Test ON, Depth Write OFF 씬 지오메트리에 가려지지만 씬 깊이에 영향을 주지 않습니다.
 * - 두꺼운 라인 지원 계획: 현재 LINELIST(1px) 사용. 추후 TRIANGLELIST+CPU quad-expand로 전환 예정.
 *   API(DrawDebugLine)는 변경 없음, DebugLinePass 내부만 교체하면 됩니다.
 */
class SE_CORE_API DebugLinePass : public RenderPassBase
{
    SE_CLASS(DebugLinePass, RenderPassBase)

public:
    DebugLinePass(
        DebugDrawSubsystem& in_debug_subsystem,
        const RenderView& in_render_view,
        RGTextureHandle in_color_target,
        RGTextureHandle in_depth_target
    );

    virtual void Setup(RGSetupContext& context) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    DebugDrawSubsystem& debug_subsystem;
    RenderView render_view;
    RGTextureHandle color_target_handle;
    RGTextureHandle depth_target_handle;
};
} // namespace se
