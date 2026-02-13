#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"


namespace se::ecs
{
class World;
}

namespace se::graphics
{
/** 개별 객체의 렌더링 정보 */
struct EntityDrawInfo
{
    Matrix4x4 mvp_matrix;
    asset::AssetId mesh_id;
    asset::AssetId material_id;
};

/**
 * World에 있는 Entity를 그리는 Pass
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) ForwardScenePass : public RenderPassBase
{
    SE_CLASS(ForwardScenePass, RenderPassBase)

public:
    explicit ForwardScenePass(
        ecs::World& in_world_ref,
        const Matrix4x4& in_vp_matrix,
        const StringName& in_color_target_name,
        const StringName& in_depth_target_name,
        uint32 in_width, uint32 in_height
    );

    virtual void Setup(RenderGraphBuilder& builder) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    const Matrix4x4 vp_matrix;
    ecs::World& world_ref; // TODO: 추후 QueryEntities가 const로 바뀌면 const&로 수정
    const StringName color_target_name;
    const StringName depth_target_name;
    const uint32 width;
    const uint32 height;

    Array<EntityDrawInfo> draw_infos;
    RGResourceHandle color_target_handle;
    RGResourceHandle depth_target_handle;
};
}  // namespace se::graphics
