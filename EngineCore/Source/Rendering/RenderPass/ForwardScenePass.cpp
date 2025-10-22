#include "Rendering/RenderPass/ForwardScenePass.h"

#include "Core/Logging/Logging.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "World/Query.h"
#include "World/World.h"
#include "World/Components/Camera3dComponent.h"
#include "World/Components/MaterialHandleComponent.h"
#include "World/Components/MeshHandleComponent.h"
#include "World/Components/TransformComponent.h"

#include "SDL3/SDL_gpu.h"

using namespace se::math;
using namespace se::world;


namespace
{
se::rendering::RGResourceHandle color_target_handle;
se::rendering::RGResourceHandle depth_target_handle;
}

namespace se::rendering
{
StringName ForwardScenePass::SceneColorTarget = u8"SceneColorTarget";
StringName ForwardScenePass::SceneDepthTarget = u8"SceneDepthTarget";

ForwardScenePass::ForwardScenePass(World& world, uint32 width, uint32 height)
    : width(width)
    , height(height)
    , world_ref(world)
{
}

void ForwardScenePass::Setup(RenderGraphBuilder& builder)
{
    /**
     * 1. 월드에서 렌더링에 필요한 Entity목록을 가져옴
     */
    // 카메라 행렬 정보 생성
    Matrix4x4 vp_matrix;

    Query camera_query = world_ref.QueryEntities<const TransformComponent&, const Camera3dComponent&>();
    if (Optional camera_opt = camera_query.GetSingle())
    {
        const auto& [camera_transform, camera] = *camera_opt;

        vp_matrix = TransformUtility::MakeViewMatrix(
            camera_transform.position,
            camera_transform.position + camera_transform.rotation.GetForwardVector(),
            Vector3::Up()
        ) * TransformUtility::MakePerspectiveMatrix(
            Radian{ camera.fov },
            static_cast<double>(width) / static_cast<double>(height),
            camera.near_plane,
            camera.far_plane
        );
    }
    else
    {
        vp_matrix = TransformUtility::MakeViewMatrix(
            Vector3::Zero(),
            Vector3::Forward(),
            Vector3::Up()
        ) * TransformUtility::MakePerspectiveMatrix(
            MathUtility::DegreesToRadians(90.0),
            static_cast<double>(width) / static_cast<double>(height),
            1.0,
            10000.0
        );
    }

    // 엔티티의 렌더 정보 생성
    draw_infos.clear();

    Query entity_query = world_ref.QueryEntities<const TransformComponent&, const MeshHandleComponent&, const MaterialHandleComponent&>();
    for (const auto& [transform, mesh, material] : entity_query)
    {
        draw_infos.push_back({
            .mesh_id = mesh.mesh_id,
            .material_id = material.material_id,
            .mvp_matrix = TransformUtility::MakeModelMatrix(
                transform.position,
                transform.rotation,
                transform.scale
            ) * vp_matrix
        });
    }

    /**
     * 2. Pass에서 사용할 Texture를 생성
     */
    color_target_handle = builder.CreateTexture(SceneColorTarget, {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    });
    depth_target_handle = builder.CreateTexture(SceneDepthTarget, {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    });
}

void ForwardScenePass::Execute(RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* color_target = context.GetActualTexture(color_target_handle);
    SDL_GPUTexture* depth_target = context.GetActualTexture(depth_target_handle);

    assert(color_target && "SceneColor texture is missing.");
    assert(depth_target && "SceneDepth texture is missing.");

    // TODO: Pipeline 구축

    const SDL_GPUColorTargetInfo color_target_info{
        .texture = color_target,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = { 0.2f, 0.2f, 0.2f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    const SDL_GPUDepthStencilTargetInfo depth_stencil_target_info{
        .texture = depth_target,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .clear_stencil = 0,
    };

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &color_target_info, 1, &depth_stencil_target_info);
    {
        for (const EntityDrawInfo& info : draw_infos)
        {
            // TODO: Entity Rendering
        }
    }
    SDL_EndGPURenderPass(pass);
}
}
