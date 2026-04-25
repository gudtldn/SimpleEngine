#include "SimpleEditor/WorldGrid/WorldGridPass.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"


namespace se::editor
{
WorldGridPass::WorldGridPass(
    const graphics::RenderView& in_render_view,
    graphics::RGTextureHandle in_color_target_handle,
    graphics::RGTextureHandle in_depth_target_handle
)
    : render_view(in_render_view)
    , color_target_handle(in_color_target_handle)
    , depth_target_handle(in_depth_target_handle)
{
}

void WorldGridPass::Setup(graphics::RGSetupContext& context)
{
    context.Write(color_target_handle);
    context.Write(depth_target_handle);
}

void WorldGridPass::Execute(graphics::RGExecutionContext& context)
{
    SDL_GPUCommandBuffer* cmd = context.GetCommandBuffer();

    SDL_GPUTexture* color_target = context.GetActualTexture(color_target_handle);
    SDL_GPUTexture* depth_target = context.GetActualTexture(depth_target_handle);
    if (!(color_target && depth_target))
    {
        return;
    }

    SDL_GPUGraphicsPipeline* pipeline = [&context]
    {
        SDL_GPUColorTargetDescription color_target_desc[] = {
            {
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
                .blend_state = {
                    // ====================================================================
                    // [1단계] 컬러 섞기 (RGB)
                    // ====================================================================
                    // 공식: (Source * SrcFactor) [OP] (Destination * DstFactor)
                    // - Source (Src) : 지금 셰이더(PS)에서 붓에 묻혀온 "새 물감"
                    // - Destination (Dst) : 이미 화면(텍스처)에 칠해져 있는 "기존 바탕색"

                    // 1. 내가 들고 온 새 물감(Src)은 얼마나 강하게 칠할 것인가?
                    // -> SDL_GPU_BLENDFACTOR_SRC_ALPHA:
                    //    "새 물감의 투명도(Alpha) 값만큼만 곱해서 칠해라." (알파가 0.3이면 30%만 칠함)
                    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,

                    // 2. 원래 있던 바탕색(Dst)은 얼마나 남겨둘 것인가?
                    // -> SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA:
                    //    "1에서 새 물감의 투명도를 뺀(1 - Alpha) 만큼만 남겨라." (새 물감이 30%면, 기존 바탕은 70% 보존)
                    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,

                    // 3. 위의 두 값을 어떻게 합칠 것인가?
                    // -> SDL_GPU_BLENDOP_ADD: "두 결과를 더해라(+)."
                    .color_blend_op = SDL_GPU_BLENDOP_ADD,

                    // ====================================================================
                    // [2단계] 투명도 섞기 (Alpha 채널)
                    // ====================================================================
                    // 화면 자체의 투명도를 결정합니다. (보통 최종 화면은 불투명하므로 크게 중요하지 않을 때가 많습니다)

                    // 1. 새 물감의 투명도 정보는 100%(ONE) 그대로 가져온다.
                    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,

                    // 2. 바탕색의 투명도 정보는 0%(ZERO)로 무시하고 버린다.
                    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,

                    // 3. 더한다. (결론적으로 새 텍스처의 알파 값으로 완전히 덮어쓰게 됨)
                    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,

                    // ====================================================================
                    // [3단계] 마스크 및 최종 스위치
                    // ====================================================================

                    // 쓰기 마스크: 계산된 결과를 최종적으로 R, G, B 채널에 기록하겠다.
                    .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B,

                    // 위에서 설정한 블렌딩(섞기) 기능을 실제로 켤 것인가? (true = 켜기)
                    .enable_blend = true,

                    // 위에서 설정한 색상 마스크를 사용할 것인가? (true = 켜기)
                    .enable_color_write_mask = true,
                },
            }
        };

        return context.GetOrCreateGraphicsPipeline({
            .vertex_shader = "EditorShader://WorldGrid.vert",
            .fragment_shader = "EditorShader://WorldGrid.frag",

            .vertex_input_state = {
                .num_vertex_buffers = 0,
                .num_vertex_attributes = 0,
            },

            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,

            .rasterizer_state = {
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = SDL_GPU_CULLMODE_NONE,
                .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
            },

            .multisample_state = {},

            .depth_stencil_state = {
                .compare_op = SDL_GPU_COMPAREOP_LESS,
                .enable_depth_test = true,
                .enable_depth_write = false,
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

        // Vertex UBO 업로드
        struct alignas(16) VertexUBO
        {
            Matrix4x4f vp;
            Vector3f camera_pos;
            float grid_size;
        } ubo_vert;

        ubo_vert.vp = static_cast<Matrix4x4f>(render_view.view_matrix * render_view.projection_matrix);
        ubo_vert.camera_pos = static_cast<Vector3f>(render_view.camera_pos);
        ubo_vert.grid_size = 10000.0f;
        SDL_PushGPUVertexUniformData(cmd, 0, &ubo_vert, sizeof(ubo_vert));

        // Fragment UBO 업로드
        struct alignas(16) FragmentUBO
        {
            // LOD 변경 전, 그리드 한칸이 유지해야 할 최소 픽셀 수 (보통 2.0 ~ 20.0)
            // 값이 클수록 카메라가 조금만 멀어져도 빠르게 다음 LOD로 전환됨.
            float grid_min_pixels_between_cells;

            // 가장 얇은 그리드 선 한 칸의 실제 월드 크기 (기본 1.0m)
            float grid_cell_size;
            float _padding[2];

            // 얇은 선의 색상과 투명도
            LinearColor grid_color_thin;

            // 두꺼운 선의 색상과 투명도
            LinearColor grid_color_thick;
        } ubo_frag;

        ubo_frag.grid_min_pixels_between_cells = 10.0f;
        ubo_frag.grid_cell_size = 1.0f;
        ubo_frag.grid_color_thin = LinearColor{ 0.5f, 0.5f, 0.5f, 0.25f };
        ubo_frag.grid_color_thick = LinearColor{ 0.5f, 0.5f, 0.5f, 0.5f };
        SDL_PushGPUFragmentUniformData(cmd, 0, &ubo_frag, sizeof(ubo_frag));

        // 그리기 명령 (Draw Call)
        // 정점 버퍼가 없으므로 셰이더의 Indices 배열 크기인 정점 6개를 그려달라고 요청
        SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    }
    SDL_EndGPURenderPass(pass);
}
} // namespace se::editor
