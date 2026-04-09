#include "SimpleEditor/Gizmo/GizmoPickPass.h"

#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEditor/Gizmo/GizmoRenderer.h"
#include "SimpleEditor/Gizmo/GizmoVertex.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
{
using namespace se::math;
using namespace se::graphics;

SE_BEGIN_REFLECT(GizmoPickPass, meta::Internal)
SE_END_REFLECT(GizmoPickPass)

GizmoPickPass::GizmoPickPass(
    const GizmoDrawList& in_draw_list,
    const RenderView& in_render_view,
    RGTextureHandle in_pick_target,
    Vector2f in_cursor_pos
)
    : draw_list(in_draw_list)
    , render_view(in_render_view)
    , pick_target_handle(in_pick_target)
    , cursor_pos(in_cursor_pos)
{
}

void GizmoPickPass::Setup(RGSetupContext& context)
{
    context.Write(pick_target_handle);
}

void GizmoPickPass::Execute(RGExecutionContext& context)
{
    const usize line_vertex_count = draw_list.GetLineVertexCount();
    const usize triangle_vertex_count = draw_list.GetTriangleVertexCount();
    if (line_vertex_count == 0 && triangle_vertex_count == 0)
    {
        return;
    }

    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* pick_target = context.GetActualTexture(pick_target_handle);
    if (!pick_target)
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

    // UBO 구성 (GizmoPass와 동일 레이아웃)
    GizmoUBO ubo;
    std::ranges::transform(
        pick_vp.data,
        ubo.vp.data.begin(),
        [](double v) { return static_cast<float>(v); }
    );
    ubo.gizmo_center = {
        static_cast<float>(draw_list.GetCenter().x),
        static_cast<float>(draw_list.GetCenter().y),
        static_cast<float>(draw_list.GetCenter().z),
    };
    ubo.screen_scale = static_cast<float>(GizmoRenderer::ComputeScreenScale(draw_list.GetCenter(), render_view));

    // 라인/삼각형 공통 파이프라인 설정
    SDL_GPUVertexBufferDescription vertex_buffer_desc[] = {
        {
            .slot = 0,
            .pitch = sizeof(GizmoVertex),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        }
    };

    SDL_GPUVertexAttribute vertex_attributes[] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(GizmoVertex, position),
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(GizmoVertex, color),
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_UINT,
            .offset = offsetof(GizmoVertex, pick_id),
        },
    };

    SDL_GPUColorTargetDescription color_target_desc[] = {
        { .format = SDL_GPU_TEXTUREFORMAT_R32_UINT }
    };

    const GraphicsPipelineCreateInfo base_info = {
        .vertex_shader = "EditorShader://GizmoPick.vert",
        .fragment_shader = "EditorShader://GizmoPick.frag",

        .vertex_input_state = {
            .vertex_buffer_descriptions = vertex_buffer_desc,
            .num_vertex_buffers = std::size(vertex_buffer_desc),
            .vertex_attributes = vertex_attributes,
            .num_vertex_attributes = std::size(vertex_attributes),
        },

        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,

        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },

        .multisample_state = {},

        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .enable_depth_test = false,
            .enable_depth_write = false,
            .enable_stencil_test = false,
        },

        .target_info = {
            .color_target_descriptions = color_target_desc,
            .num_color_targets = std::size(color_target_desc),
            .has_depth_stencil_target = false,
        },
    };

    // 1x1 텍스처에 렌더링 (clear to 0 = EGizmoAxis::None)
    const SDL_GPUColorTargetInfo color_target_info[] = {
        {
            .texture = pick_target,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
    };

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, std::size(color_target_info), nullptr);
    {
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

        SDL_PushGPUVertexUniformData(cmd, 0, &ubo, sizeof(ubo));

        // 라인 드로우
        if (line_vertex_count > 0)
        {
            SDL_GPUGraphicsPipeline* line_pipeline = context.GetOrCreateGraphicsPipeline(base_info);
            SDL_BindGPUGraphicsPipeline(pass, line_pipeline);

            const SDL_GPUBufferBinding vertex_binding = {
                .buffer = draw_list.GetLineVertexBuffer(),
                .offset = 0,
            };
            SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
            SDL_DrawGPUPrimitives(pass, static_cast<uint32>(line_vertex_count), 1, 0, 0);
        }

        // 삼각형 드로우
        if (triangle_vertex_count > 0)
        {
            GraphicsPipelineCreateInfo triangle_info = base_info;
            triangle_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

            SDL_GPUGraphicsPipeline* triangle_pipeline = context.GetOrCreateGraphicsPipeline(triangle_info);
            SDL_BindGPUGraphicsPipeline(pass, triangle_pipeline);

            const SDL_GPUBufferBinding vertex_binding = {
                .buffer = draw_list.GetTriangleVertexBuffer(),
                .offset = 0,
            };
            SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);
            SDL_DrawGPUPrimitives(pass, static_cast<uint32>(triangle_vertex_count), 1, 0, 0);
        }
    }
    SDL_EndGPURenderPass(pass);
}
} // namespace se::editor
