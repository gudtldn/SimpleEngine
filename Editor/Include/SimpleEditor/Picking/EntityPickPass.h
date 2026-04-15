#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Picking/EntityPickId.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::graphics
{
struct SceneDrawData;
class GpuResourceManager;
}

namespace se::editor
{
/** 개별 오브젝트의 프레임 내 렌더링 정보 (피킹용) */
struct EntityColorPickDrawInfo
{
    Matrix4x4 model_matrix;
    uint32 encoded_entity_id = ENTITY_PICK_MISS;
    asset::AssetId mesh_id;
};

/**
 * SceneDrawData의 오브젝트를 피킹하는 패스
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) EntityPickPass : public se::graphics::RenderPassBase
{
    SE_CLASS(EntityPickPass, se::graphics::RenderPassBase)

public:
    explicit EntityPickPass(
        const graphics::SceneDrawData& in_draw_data,
        const graphics::GpuResourceManager& in_gpu_manager,
        const graphics::RenderView& in_render_view,
        graphics::RGTextureHandle in_pick_target,
        graphics::RGTextureHandle in_pick_depth,
        const Vector2f& in_cursor_pos
    );

    virtual void Setup(graphics::RGSetupContext& context) override;
    virtual void Execute(graphics::RGExecutionContext& context) override;

private:
    const graphics::SceneDrawData& draw_data;
    const graphics::GpuResourceManager& gpu_manager;
    graphics::RenderView render_view;

    Array<EntityColorPickDrawInfo> draw_infos;
    graphics::RGTextureHandle pick_target_handle;
    graphics::RGTextureHandle pick_depth_handle;
    Vector2f cursor_pos;
};
} // namespace se::editor
