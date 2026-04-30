#include "SimpleEngine/Graphics/RenderPass/DebugLinePass.h"

#include "SimpleEngine/Debug/DebugDrawSubsystem.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
using namespace se::math;

SE_BEGIN_REFLECT(DebugLinePass, meta::Internal)
SE_END_REFLECT(DebugLinePass)

DebugLinePass::DebugLinePass(
    DebugDrawSubsystem& in_debug_subsystem,
    const RenderView& in_render_view,
    RGTextureHandle in_color_target,
    RGTextureHandle in_depth_target
)
    : debug_subsystem(in_debug_subsystem)
    , render_view(in_render_view)
    , color_target_handle(in_color_target)
    , depth_target_handle(in_depth_target)
{
}

void DebugLinePass::Setup(RGSetupContext& context)
{
    // ForwardScenePass위에 Draw 하기 위함
    context.Write(color_target_handle);
    context.Write(depth_target_handle);
}

void DebugLinePass::Execute(RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    // 이번 프레임 라인을 GPU 버퍼에 업로드 (없으면 패스 전체 건너뜀)
    const usize line_count = debug_subsystem.GetLineCount();
    if (line_count == 0)
    {
        return;
    }

    SDL_GPUTexture* color_target = context.GetActualTexture(color_target_handle);
    SDL_GPUTexture* depth_target = context.GetActualTexture(depth_target_handle);
    if (!(color_target && depth_target))
    {
        return;
    }

    // color_target 위에 라인을 Draw
    const SDL_GPUColorTargetInfo color_target_info[] = {
        {
            .texture = color_target,
            .load_op = SDL_GPU_LOADOP_LOAD,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
    };

    // Depth는 Read-only (Test ON, Write OFF)
    const SDL_GPUDepthStencilTargetInfo depth_target_info = {
        .texture = depth_target,
        .load_op = SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_LOAD,
        .stencil_store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPUGraphicsPipeline* pipeline = [&context]
    {
        SDL_GPUVertexBufferDescription vertex_buffer_desc[] = {
            {
                .slot = 0,
                .pitch = sizeof(DebugVertex),
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            }
        };

        SDL_GPUVertexAttribute vertex_attributes[] = {
            {
                .location = 0,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .offset = offsetof(DebugVertex, position),
            },
            {
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = offsetof(DebugVertex, color),
            },
        };

        SDL_GPUColorTargetDescription color_target_desc[] = {
            { .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB }
        };

        return context.GetOrCreateGraphicsPipeline({
            .vertex_shader = "CoreShader://DebugLine.vert",
            .fragment_shader = "CoreShader://DebugLine.frag",

            .vertex_input_state = {
                .vertex_buffer_descriptions = vertex_buffer_desc,
                .num_vertex_buffers = std::size(vertex_buffer_desc),
                .vertex_attributes = vertex_attributes,
                .num_vertex_attributes = std::size(vertex_attributes),
            },

            .primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST,

            .rasterizer_state = {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = SDL_GPU_CULLMODE_NONE, // 라인은 컬링이 필요없음
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            },

            .multisample_state = {},

            .depth_stencil_state = {
                .compare_op = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_test = true,
                .enable_depth_write = false, // Depth 버퍼 수정 X
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

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, std::size(color_target_info), &depth_target_info);
    {
        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        const SDL_GPUViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .w = static_cast<float>(render_view.width),
            .h = static_cast<float>(render_view.height),
            .min_depth = 0.0f, .max_depth = 1.0f,
        };
        const SDL_Rect scissor = {
            .x = 0, .y = 0,
            .w = static_cast<int32>(render_view.width),
            .h = static_cast<int32>(render_view.height),
        };
        SDL_SetGPUViewport(pass, &viewport);
        SDL_SetGPUScissor(pass, &scissor);

        // VP 행렬 업로드
        const Matrix4x4 vp = render_view.view_matrix * render_view.projection_matrix;
        Matrix4x4f vpf;
        std::ranges::transform(
            vp.data,
            vpf.data.begin(),
            [](double v) { return static_cast<float>(v); }
        );
        SDL_PushGPUVertexUniformData(cmd, 0, &vpf, sizeof(vpf));

        const SDL_GPUBufferBinding vertex_binding = {
            .buffer = debug_subsystem.GetVertexBuffer(),
            .offset = 0,
        };
        SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);

        // 라인 1개 = 정점 2개
        SDL_DrawGPUPrimitives(pass, static_cast<uint32>(line_count) * 2, 1, 0, 0);
    }
    SDL_EndGPURenderPass(pass);
}
} // namespace se
