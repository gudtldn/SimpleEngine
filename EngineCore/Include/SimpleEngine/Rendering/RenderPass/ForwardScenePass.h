#pragma once
#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Types/StringName.h"
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
    static StringName SceneColorTarget;
    static StringName SceneDepthTarget;

public:
    explicit ForwardScenePass(world::World& world, uint32 width, uint32 height);

    virtual void Setup(RenderGraphBuilder& builder) override;
    virtual void Execute(RGExecutionContext& context) override;

private:
    const uint32 width;
    const uint32 height;

    world::World& world_ref;
    vector<EntityDrawInfo> draw_infos;
};
}
