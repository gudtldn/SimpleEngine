#include "SimpleEditor/Gizmo/GizmoPass.h"

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

SE_BEGIN_REFLECT(GizmoPass, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(GizmoPass)

GizmoPass::GizmoPass(
    const GizmoDrawList& in_draw_list,
    const RenderView& in_render_view,
    RGTextureHandle in_color_target,
    RGTextureHandle in_depth_target
)
    : draw_list(in_draw_list)
    , render_view(in_render_view)
    , color_target_handle(in_color_target)
    , depth_target_handle(in_depth_target)
{
}

void GizmoPass::Setup(RGSetupContext& context)
{
    context.Write(color_target_handle);
    context.Write(depth_target_handle);
}

void GizmoPass::Execute(RGExecutionContext& context)
{
    const usize line_vertex_count = draw_list.GetLineVertexCount();
    const usize triangle_vertex_count = draw_list.GetTriangleVertexCount();
    if (line_vertex_count == 0 && triangle_vertex_count == 0)
    {
        return;
    }

    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* color_target = context.GetActualTexture(color_target_handle);
    SDL_GPUTexture* depth_target = context.GetActualTexture(depth_target_handle);
    if (!(color_target && depth_target))
    {
        return;
    }

    const SDL_GPUColorTargetInfo color_target_info[] = {
        {
            .texture = color_target,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
    };

    const SDL_GPUDepthStencilTargetInfo depth_target_info = {
        .texture = depth_target,
        .load_op = SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_LOAD,
        .stencil_store_op = SDL_GPU_STOREOP_STORE,
    };

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
        { .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB }
    };

    const GraphicsPipelineCreateInfo base_info = {
        .vertex_shader = "EditorShader://Gizmo.vert",
        .fragment_shader = "EditorShader://Gizmo.frag",

        .vertex_input_state = {
            .vertex_buffer_descriptions = vertex_buffer_desc,
            .num_vertex_buffers = std::size(vertex_buffer_desc),
            .vertex_attributes = vertex_attributes,
            .num_vertex_attributes = std::size(vertex_attributes),
        },

        // 라인용 기본값, Triangle은 아래에서 오버라이드
        .primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,

        .rasterizer_state = {
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        },

        .multisample_state = {},

        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_ALWAYS,
            .enable_depth_test = false, // 항상 씬 위에 렌더링
            .enable_depth_write = false,
            .enable_stencil_test = false,
        },

        .target_info = {
            .color_target_descriptions = color_target_desc,
            .num_color_targets = std::size(color_target_desc),
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
            .has_depth_stencil_target = true,
        },
    };

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, std::size(color_target_info), &depth_target_info);
    {
        const SDL_GPUViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .w = static_cast<f32>(render_view.width),
            .h = static_cast<f32>(render_view.height),
            .min_depth = 0.0f, .max_depth = 1.0f,
        };
        const SDL_Rect scissor = {
            .x = 0, .y = 0,
            .w = static_cast<i32>(render_view.width),
            .h = static_cast<i32>(render_view.height),
        };
        SDL_SetGPUViewport(pass, &viewport);
        SDL_SetGPUScissor(pass, &scissor);

        // UBO 업로드: VP + GizmoCenter + ScreenScale
        const Matrix4x4 vp = render_view.view_matrix * render_view.projection_matrix;

        GizmoUBO ubo;
        ubo.vp = static_cast<Matrix4x4f>(vp);
        ubo.gizmo_center = static_cast<Vector3f>(draw_list.GetCenter());
        ubo.screen_scale = static_cast<f32>(GizmoRenderer::ComputeScreenScale(draw_list.GetCenter(), render_view));

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
            SDL_DrawGPUPrimitives(pass, static_cast<u32>(line_vertex_count), 1, 0, 0);
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
            SDL_DrawGPUPrimitives(pass, static_cast<u32>(triangle_vertex_count), 1, 0, 0);
        }
    }
    SDL_EndGPURenderPass(pass);
}
} // namespace se::editor
