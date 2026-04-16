#include "SimpleEditor/Picking/EntityPickPass.h"
#include "SimpleEditor/Picking/EntityPickId.h"

#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"
#include "SimpleEngine/Graphics/Scene/DrawCommand.h"
#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"


namespace se::editor
{
EntityPickPass::EntityPickPass(
    const graphics::SceneDrawData& in_draw_data,
    const graphics::GpuResourceManager& in_gpu_manager,
    const graphics::RenderView& in_render_view,
    graphics::RGTextureHandle in_pick_target,
    graphics::RGTextureHandle in_pick_depth,
    const Vector2f& in_cursor_pos
)
    : draw_data(in_draw_data)
    , gpu_manager(in_gpu_manager)
    , render_view(in_render_view)
    , pick_target_handle(in_pick_target)
    , pick_depth_handle(in_pick_depth)
    , cursor_pos(in_cursor_pos)
{
}

void EntityPickPass::Setup(graphics::RGSetupContext& context)
{
    context.Write(pick_target_handle);
    context.Write(pick_depth_handle);
}

void EntityPickPass::Execute(graphics::RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* pick_target = context.GetActualTexture(pick_target_handle);
    SDL_GPUTexture* pick_depth = context.GetActualTexture(pick_depth_handle);
    if (!(pick_target && pick_depth))
    {
        return;
    }

    // ========================================================================
    // [Pick Matrix 구성] "1x1 스코프 투영"
    // 마우스 커서 위치의 단 1픽셀 영역을 화면 전체(NDC 공간) 크기로 엄청나게 확대합니다.
    // 이 변환을 거치면 커서 영역 밖의 모든 기즈모 폴리곤은 클리핑(Clipping) 단계에서
    // 하드웨어적으로 폐기되므로, 픽셀 셰이더 부하를 0에 가깝게 최적화할 수 있습니다.
    // ========================================================================
    const double vp_w = static_cast<double>(render_view.width);
    const double vp_h = static_cast<double>(render_view.height);
    const double cx = static_cast<double>(cursor_pos.x);
    const double cy = static_cast<double>(cursor_pos.y);

    // Row-vector기반 이동/스케일 행렬 (p' = p * M)
    // | W       0       0   0 |
    // | 0       H       0   0 |
    // | 0       0       1   0 |
    // | W-2*cx  H-2*cy  0   1 |
    Matrix4x4 pick_matrix = Matrix4x4::Identity();

    // 1. 화면 해상도만큼 스케일을 키워 1픽셀을 NDC 전체 크기로 확대
    pick_matrix[0, 0] = vp_w;
    pick_matrix[1, 1] = vp_h;

    // 2. 마우스 커서 위치(cx, cy)를 NDC의 정중앙(0, 0)으로 이동
    pick_matrix[3, 0] = vp_w - (2.0 * cx);
    pick_matrix[3, 1] = -(vp_h - (2.0 * cy)); // SDL_GPU의 화면 좌표계는 Top-Left 기준(Y-down)이므로 Y축 이동 공식의 부호를 반전

    const Matrix4x4 pick_vp = render_view.view_matrix * render_view.projection_matrix * pick_matrix;


    // PSO: 씬 메시 Vertex 레이아웃에서 position만 바인딩 (stride는 원본 유지)
    // -> normal/uv/tangent는 GPU가 stride 간격으로 건너뜀
    SDL_GPUGraphicsPipeline* pipeline = [&]
    {
        SDL_GPUVertexBufferDescription vertex_buffer_desc[] = {
            {
                .slot = 0,
                .pitch = sizeof(graphics::Vertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            }
        };

        SDL_GPUVertexAttribute vertex_attributes[] = {
            {
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .offset = offsetof(graphics::Vertex, position),
            },
        };

        SDL_GPUColorTargetDescription color_target_desc[] = {
            { .format = SDL_GPU_TEXTUREFORMAT_R32_UINT }
        };

        return context.GetOrCreateGraphicsPipeline({
            .vertex_shader = "EditorShader://EntityPick.vert",
            .fragment_shader = "EditorShader://EntityPick.frag",

            .vertex_input_state = {
                .vertex_buffer_descriptions = vertex_buffer_desc,
                .num_vertex_buffers = std::size(vertex_buffer_desc),
                .vertex_attributes = vertex_attributes,
                .num_vertex_attributes = std::size(vertex_attributes),
            },

            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

            .rasterizer_state = {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = SDL_GPU_CULLMODE_BACK,
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            },

            .multisample_state = {},

            .depth_stencil_state = {
                .compare_op = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_test = true,
                .enable_depth_write = true,
                .enable_stencil_test = false,
            },

            .target_info = {
                .color_target_descriptions = color_target_desc,
                .num_color_targets = std::size(color_target_desc),
                .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                .has_depth_stencil_target = true,
            },
        });
    }();

    const SDL_GPUColorTargetInfo color_target_info[] = {
        {
            .texture = pick_target,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f }, // 정확한 Entity Id는 Encode, Decode에서 +-1 연산
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
    };
    const SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
        .texture = pick_depth,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .clear_stencil = 0,
    };

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, std::size(color_target_info), &depth_stencil_target_info);
    {
        // PSO 설정
        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        // Viewport/Scissor 설정
        constexpr SDL_GPUViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .w = 1.0f, .h = 1.0f,
            .min_depth = 0.0f, .max_depth = 1.0f,
        };

        constexpr SDL_Rect scissor = {
            .x = 0, .y = 0,
            .w = 1, .h = 1,
        };

        SDL_SetGPUViewport(pass, &viewport);
        SDL_SetGPUScissor(pass, &scissor);

        // double -> float 변환 헬퍼
        auto to_float4x4 = [](const Matrix4x4& src, Matrix4x4f& dst)
        {
            std::ranges::transform(
                src.data,
                dst.data.begin(),
                [](double v) { return static_cast<float>(v); }
            );
        };

        // Vertex Uniform slot 0: Pick VP 행렬 (per-pass, 모든 오브젝트 공유)
        // pick_matrix 적용으로 커서 1px 영역만 NDC 전체로 확대됨
        struct alignas(16) PassUBO
        {
            Matrix4x4f vp;
        } pass_ubo;
        to_float4x4(pick_vp, pass_ubo.vp);
        SDL_PushGPUVertexUniformData(cmd, 0, &pass_ubo, sizeof(pass_ubo));

        // Draw Meshes
        for (const graphics::DrawCommand& draw_cmd : draw_data.opaque_commands)
        {
            const auto slice = gpu_manager.GetSlice(draw_cmd.mesh_id);
            if (!slice.HasValue())
            {
                continue;
            }

            // Vertex Buffer 바인딩
            // 셰이더의 Input Slot 0번에 바인딩
            const SDL_GPUBufferBinding vertex_binding = {
                .buffer = slice->buffer,
                .offset = slice->offset
            };
            SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);

            if (slice->index_count > 0)
            {
                // Index Buffer 바인딩
                const SDL_GPUBufferBinding index_binding = {
                    .buffer = slice->buffer,
                    .offset = slice->index_offset
                };
                SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            }

            // Vertex Uniform slot 1: Model 행렬 + entity_id (per-object)
            // alignas(16): HLSL cbuffer는 16바이트 행 단위 패킹 -> sizeof = 80
            // TODO: 추후 RTE(Relative To Eye) 방식으로 수정
            struct alignas(16) ObjectUBO
            {
                Matrix4x4f model; // 64 bytes (4 x float4)
                uint32 entity_id; // 4 bytes + 12 bytes padding (= 1 x float4)
            } object_ubo;
            to_float4x4(draw_cmd.model_matrix, object_ubo.model);
            object_ubo.entity_id = EntityPickId::Encode(draw_cmd.entity_id).encoded;
            SDL_PushGPUVertexUniformData(cmd, 1, &object_ubo, sizeof(object_ubo));

            if (slice->index_count > 0)
            {
                SDL_DrawGPUIndexedPrimitives(pass, slice->index_count, 1, 0, 0, 0);
            }
            else
            {
                const uint32 vertex_count = (slice->index_offset - slice->offset) / sizeof(graphics::Vertex);
                SDL_DrawGPUPrimitives(pass, vertex_count, 1, 0, 0);
            }
        }
    }
    SDL_EndGPURenderPass(pass);
}
} // namespace se::editor
