#include "Rendering/RenderPass/ForwardScenePass.h"

#include "Core/Logging/Logging.h"
#include "Core/Types/VPath.h"
#include "Geometry/Vertex.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "World/Query.h"
#include "World/World.h"
#include "World/Components/Camera3dComponent.h"
#include "World/Components/MaterialHandleComponent.h"
#include "World/Components/MeshHandleComponent.h"
#include "World/Components/TransformComponent.h"

#include "SDL3/SDL_gpu.h"
#include "Utility/PathResolver.h"

using namespace se::math;
using namespace se::world;


namespace
{
se::rendering::RGResourceHandle color_target_handle;
se::rendering::RGResourceHandle depth_target_handle;
}

namespace se::rendering
{
StringName ForwardScenePass::SceneColorTarget = "SceneColorTarget";
StringName ForwardScenePass::SceneDepthTarget = "SceneDepthTarget";

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
    draw_infos.Clear();

    Query entity_query = world_ref.QueryEntities<const TransformComponent&, const MeshHandleComponent&, const MaterialHandleComponent&>();
    for (const auto [transform, mesh, material] : entity_query)
    {
        draw_infos.Push({
            .mvp_matrix = TransformUtility::MakeModelMatrix(
                transform.position,
                transform.rotation,
                transform.scale
            ) * vp_matrix,
            .mesh_id = mesh.mesh_id,
            .material_id = material.material_id,
        });
    }

    /**
     * 2. Pass에서 사용할 Texture를 생성
     */
    // color_target_handle = builder.CreateTexture(SceneColorTarget, {
    //     .type = SDL_GPU_TEXTURETYPE_2D,                      // 2D 텍스처
    //     .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB, // RGBA 각 채널당 8비트, sRGB 색 공간 사용
    //     // 이 텍스처의 사용 목적
    //     // - COLOR_TARGET: 이 텍스처에 그림을 그릴(렌더링할) 것임을 의미
    //     // - SAMPLER: 나중에 다른 패스(예: 후처리, UI)에서 이 텍스처를 읽어서 사용할 것임을 의미
    //     .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
    //     .width = width,
    //     .height = height,
    //     .layer_count_or_depth = 1, // 2D 텍스처이므로 레이어는 1개
    //     .num_levels = 1,
    //     .sample_count = SDL_GPU_SAMPLECOUNT_1,
    // });
    color_target_handle = builder.GetResourceHandleByName(SceneColorTarget);
    builder.Write(color_target_handle);

    // depth_target_handle = builder.CreateTexture(SceneDepthTarget, {
    //     .type = SDL_GPU_TEXTURETYPE_2D,
    //     .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,  // 24비트 깊이버퍼 + 8비트 스텐실버퍼
    //     .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, // 이 텍스처는 깊이/스텐실 버퍼로만 사용될 것임을 의미
    //     .width = width,
    //     .height = height,
    //     .layer_count_or_depth = 1,
    //     .num_levels = 1,
    //     .sample_count = SDL_GPU_SAMPLECOUNT_1,
    // });
    depth_target_handle = builder.GetResourceHandleByName(SceneDepthTarget);
    builder.Write(depth_target_handle);
}

void ForwardScenePass::Execute(RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* color_target = context.GetActualTexture(color_target_handle);
    SDL_GPUTexture* depth_target = context.GetActualTexture(depth_target_handle);

    if (!(color_target && depth_target)) { return; }

    const SDL_GPUColorTargetInfo color_target_info[] = {
        {
            .texture = color_target,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { 0.15f, 0.15f, 0.15f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
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

    SDL_GPUGraphicsPipeline* pipeline;
    {
        // TODO: 여기서 셰이더 컴파일하면 프레임 드랍이 생길 수 있음, 개선필요
        static const std::filesystem::path VSPath = utility::PathResolver::Get().Resolve("CoreShader://Default.vert.hlsl").Value();
        static const std::filesystem::path FSPath = utility::PathResolver::Get().Resolve("CoreShader://Default.frag.hlsl").Value();

        /**
         * 정점 버퍼(Vertex Buffer) 자체에 대한 Description
         * 버퍼가 여러 개일 경우, 각 버퍼에 대한 정보를 여기에 정의한다
         */
        SDL_GPUVertexBufferDescription vertex_buffer_desc[] = {
            {
                .slot = 0,                                    // 이 버퍼가 바인딩될 슬롯 번호 (셰이더에서 참조)
                .pitch = sizeof(Vertex),                      // 정점 하나가 차지하는 총 메모리 크기 (stride)
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, // 버퍼 데이터가 정점마다 바뀌는지(VERTEX) 또는 인스턴스마다 바뀌는지(INSTANCE)
            },
        };

        /**
         * 정점 버퍼 내부의 각 데이터(속성, attribute)가 무엇을 의미하는지 설정
         * Vertex 셰이더의 입력(VertexInput) 구조체와 정확히 일치해야 한다
         */
        SDL_GPUVertexAttribute vertex_attributes[] = {
            {
                .location = 0,                                // 셰이더 내에서의 위치(location). HLSL의 :POSITION에 해당
                .buffer_slot = 0,                             // 이 속성이 어느 버퍼(vertex_buffer_desc)에 속하는지
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, // 데이터 타입 (float 4개)
                .offset = offsetof(Vertex, position)          // Vertex 구조체 내에서 이 속성이 시작되는 위치(offset)
            },
            {
                .location = 1, // 셰이더 내에서의 위치. HLSL의 :COLOR0에 해당
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = offsetof(Vertex, color)
            },
        };

        /**
         * 컬러 렌더 타겟에 대한 Description
         * 파이프라인이 어떤 포맷의 텍스처에 렌더링될 것인지, 그리고 어떻게 색상을 혼합(블렌딩)할지 정의한다
         */
        SDL_GPUColorTargetDescription color_target_desc[] = {
            {
                // 이 파이프라인이 렌더링할 텍스처의 포맷. CreateTexture에서 사용한 포맷과 일치해야 함
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
                .blend_state = {
                    // // --- 컬러(RGB) 채널 블렌딩 설정 ---
                    // // FinalRGB = (SourceRGB * SrcColorFactor) + (DestinationRGB * DstColorFactor)
                    // .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,           // Source Color에 Source의 Alpha 값을 곱함
                    // .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, // Destination Color에 (1 - Source Alpha) 값을 곱함
                    // .color_blend_op = SDL_GPU_BLENDOP_ADD,                            // 두 결과를 더함
                    //
                    // // --- 알파(A) 채널 블렌딩 설정 ---
                    // // FinalAlpha = (SourceAlpha * SrcAlphaFactor) + (DestinationAlpha * DstAlphaFactor)
                    // .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,                 // Source Alpha는 그대로 (1을 곱함)
                    // .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, // Destination Alpha에 (1 - Source Alpha) 값을 곱함
                    // .alpha_blend_op = SDL_GPU_BLENDOP_ADD,                            // 두 결과를 더함
                    //
                    // // 모든 컬러 채널(RGBA)에 쓰기 허용
                    // .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,

                    // 블렌딩 비활성화
                    .enable_blend = false,
                },
            }
        };

        pipeline = context.GetOrCreateGraphicsPipeline({
            // 사용할 셰이더 지정
            .vertex_shader_request = { .source_path = VSPath, },
            .fragment_shader_request = { .source_path = FSPath, },

            // 정점 데이터 형식 정의
            .vertex_input_state = {
                .vertex_buffer_descriptions = vertex_buffer_desc,
                .num_vertex_buffers = std::size(vertex_buffer_desc),
                .vertex_attributes = vertex_attributes,
                .num_vertex_attributes = std::size(vertex_attributes),
            },

            // 프리미티브(기본 도형) 타입 설정
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

            // 래스터라이저(Rasterizer) 상태 설정: 3D 모델을 2D 픽셀로 변환하는 방법을 제어
            .rasterizer_state = {
                .fill_mode = SDL_GPU_FILLMODE_FILL,               // 삼각형 내부를 색으로 채움 (FILL) or 선으로만 그림 (LINE)
                .cull_mode = SDL_GPU_CULLMODE_BACK,               // 카메라를 등지고 있는 삼각형(뒷면)은 그리지 않음 (성능 최적화)
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE // 정점이 반시계 방향으로 정의된 삼각형을 앞면으로 간주
            },

            // 멀티샘플링(MSAA) 상태 설정
            .multisample_state = {},

            // 깊이/스텐실 테스트 상태 설정
            .depth_stencil_state = {
                .compare_op = SDL_GPU_COMPAREOP_LESS, // 새로 그릴 픽셀의 깊이 값이 기존 픽셀보다 작을(가까울) 때만 그림
                .enable_depth_test = true,            // 깊이 테스트를 활성화
                .enable_depth_write = true,           // 깊이 테스트를 통과한 픽셀의 깊이 값을 깊이 버퍼에 기록
                .enable_stencil_test = false,         // 스텐실 테스트는 사용하지 않음
            },

            // 렌더 타겟 정보 설정
            .target_info = {
                .color_target_descriptions = color_target_desc,
                .num_color_targets = std::size(color_target_desc),
                .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                .has_depth_stencil_target = true,
            },
        });
    }

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, std::size(color_target_info), &depth_stencil_target_info);
    {
        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        for (const EntityDrawInfo& info : draw_infos)
        {
            // TODO: Entity Rendering
        }
    }
    SDL_EndGPURenderPass(pass);
}
}
