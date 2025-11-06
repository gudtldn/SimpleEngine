#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Rendering/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Rendering/RenderPass/IRenderPass.h"


namespace se::world
{
class World;
}

namespace se::rendering
{
/** 개별 객체의 렌더링 정보 */
struct EntityDrawInfo
{
    Matrix4x4 mvp_matrix;
    uint32 mesh_id;
    uint32 material_id;
};

/**
 * World에 있는 Entity를 그리는 Pass
 */
class SE_CORE_API ForwardScenePass : public IRenderPass
{
public:
    explicit ForwardScenePass(
        world::World& world,
        const StringName& in_color_target_name,
        const StringName& in_depth_target_name,
        uint32 width, uint32 height
    );

    virtual void Setup(RenderGraphBuilder& builder) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    world::World& world_ref; // TODO: 추후 QueryEntities가 const로 바뀌면 const&로 수정
    const StringName color_target_name;
    const StringName depth_target_name;
    const uint32 width;
    const uint32 height;

    Array<EntityDrawInfo> draw_infos;
    RGResourceHandle color_target_handle;
    RGResourceHandle depth_target_handle;
};
}
