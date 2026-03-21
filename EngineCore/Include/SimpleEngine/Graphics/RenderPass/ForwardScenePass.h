#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"


namespace se::graphics
{
// forward declaration
struct SceneDrawData;
struct RenderView;
class GpuResourceManager;

/** 개별 오브젝트의 프레임 내 렌더링 정보 */
struct EntityDrawInfo
{
    Matrix4x4 mvp_matrix;
    asset::AssetId mesh_id;
    asset::AssetId material_id;
};

/**
 * SceneDrawData의 오브젝트를 Forward 렌더링하는 패스
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) ForwardScenePass : public RenderPassBase
{
    SE_CLASS(ForwardScenePass, RenderPassBase)

public:
    explicit ForwardScenePass(
        const SceneDrawData& in_draw_data,
        const RenderView& in_render_view,
        const GpuResourceManager& in_gpu_manager,
        RGResourceHandle in_color_target,
        RGResourceHandle in_depth_target
    );

    virtual void Setup(RenderGraphBuilder& builder) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    const SceneDrawData& draw_data;
    const RenderView& render_view;
    const GpuResourceManager& gpu_manager;

    Array<EntityDrawInfo> draw_infos;
    RGResourceHandle color_target_handle;
    RGResourceHandle depth_target_handle;
};
} // namespace se::graphics
