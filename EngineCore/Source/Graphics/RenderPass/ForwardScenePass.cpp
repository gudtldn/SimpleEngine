#include "SimpleEngine/Graphics/RenderPass/ForwardScenePass.h"

#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraph.h"
#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"
#include "SimpleEngine/Graphics/View/RenderView.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
using namespace se::math;

SE_BEGIN_REFLECT(ForwardScenePass, meta::Internal)
SE_END_REFLECT(ForwardScenePass)

ForwardScenePass::ForwardScenePass(
    const SceneDrawData& in_draw_data,
    const RenderView& in_render_view,
    const GpuResourceManager& in_gpu_manager
)
    : draw_data(in_draw_data)
    , render_view(in_render_view)
    , gpu_manager(in_gpu_manager)
{
}

void ForwardScenePass::Setup(RenderGraphBuilder& builder)
{
    // VP 행렬 계산 (per-view)
    const Matrix4x4 vp_matrix = render_view.view_matrix * render_view.projection_matrix;

    // DrawCommand -> EntityDrawInfo 변환 (MVP 사전 계산)
    draw_infos.Clear();
    for (const DrawCommand& cmd : draw_data.opaque_commands)
    {
        draw_infos.Push({
            .mvp_matrix = cmd.model_matrix * vp_matrix,
            .mesh_id = cmd.mesh_id,
            .material_id = cmd.material_id,
        });
    }

    // 렌더 타겟 설정
    color_target_handle = builder.GetResourceHandleByName(render_view.color_target_name);
    builder.Write(color_target_handle);

    depth_target_handle = builder.CreateTexture(render_view.depth_target_name, {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,  // 24비트 깊이버퍼 + 8비트 스텐실버퍼
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, // 이 텍스처는 깊이/스텐실 버퍼로만 사용될 것임을 의미
        .width = render_view.width,
        .height = render_view.height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    });
    builder.Write(depth_target_handle);
}

void ForwardScenePass::Execute(RGExecutionContext& context)
{
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
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.15f, .g = 0.15f, .b = 0.15f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        }
    };
    const SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
        .texture = depth_target,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .clear_stencil = 0,
    };

    SDL_GPUGraphicsPipeline* pipeline = [&context]
    {
        // TODO: 여기서 셰이더 컴파일하면 프레임 드랍이 생길 수 있음, 개선필요
        static const Path VSPath = VFS::ToPath(VPath("CoreShader://Default.vert.hlsl"));
        static const Path FSPath = VFS::ToPath(VPath("CoreShader://Default.frag.hlsl"));

        /**
         * 정점 버퍼(Vertex Buffer) 자체에 대한 Description
         * 버퍼가 여러 개일 경우, 각 버퍼에 대한 정보를 여기에 정의
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
         * Vertex 셰이더의 입력(VertexInput) 구조체와 정확히 일치해야 함
         */
        SDL_GPUVertexAttribute vertex_attributes[] = {
            {
                .location = 0,                                // 셰이더 내에서의 위치(location). HLSL의 :POSITION에 해당
                .buffer_slot = 0,                             // 이 속성이 어느 버퍼(vertex_buffer_desc)에 속하는지
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, // 데이터 타입 (float 3개)
                .offset = offsetof(Vertex, position)          // Vertex 구조체 내에서 이 속성이 시작되는 위치(offset)
            },
            {
                .location = 1,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .offset = offsetof(Vertex, normal)
            },
            {
                .location = 2,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                .offset = offsetof(Vertex, tex_coord)
            },
            {
                .location = 3,
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                .offset = offsetof(Vertex, tangent)
            },
        };

        /**
         * 렌더 타겟에 대한 Description
         * 파이프라인이 어떤 포맷의 텍스처에 렌더링될 것인지, 그리고 어떻게 색상을 혼합(블렌딩)할지 정의
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

        return context.GetOrCreateGraphicsPipeline({
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
    }();

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, std::size(color_target_info), &depth_stencil_target_info);
    {
        // PSO 설정
        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        // Viewport/Scissor 설정
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

        // Draw Meshes
        for (const EntityDrawInfo& info : draw_infos)
        {
            const GpuBufferSlice& slice = gpu_manager.GetSlice(info.mesh_id);
            if (!slice.IsValid())
            {
#if SE_BUILD_DEBUG
                static HashSet<asset::AssetId> logged_asset_ids;
                if (!logged_asset_ids.Contains(info.mesh_id))
                {
                    ConsoleLog(
                        ELogLevel::Warning,
                        "Skipping Draw: Invalid GPU slice for MeshID[{}]. Resource may not be loaded.",
                        info.mesh_id.GetGuid()
                    );
                    logged_asset_ids.Insert(info.mesh_id);
                }
#endif
                continue;
            }

            // Vertex Buffer 바인딩
            // 셰이더의 Input Slot 0번에 바인딩
            const SDL_GPUBufferBinding vertex_binding = {
                .buffer = slice.buffer,
                .offset = slice.offset
            };
            SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);

            if (slice.index_count > 0)
            {
                // Index Buffer 바인딩
                const SDL_GPUBufferBinding index_binding = {
                    .buffer = slice.buffer,
                    .offset = slice.index_offset
                };
                SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            }

            // Uniform 데이터 전송 (MVP Matrix)
            Matrix4x4f mvpf; // TODO: 추후 RTE(Relative To Eye) 방식으로 수정
            std::transform(
                info.mvp_matrix.GetData(), info.mvp_matrix.GetData() + 16,
                mvpf.GetData(),
                [](double d) { return static_cast<float>(d); }
            );
            SDL_PushGPUVertexUniformData(cmd, 0, &mvpf, sizeof(mvpf));

            // Material 바인딩 | TODO: Texture/Sampler 바인딩 함수 만들기
            // if (info.material_id.IsValid())
            // {
            //     // 텍스처 조회
            //     const GpuTexture& albedo = gpu_manager.GetTexture(material.albedo_id);
            //
            //     // 바인딩 (Sampler + Texture)
            //     if (albedo.IsValid())
            //     {
            //         const SDL_GPUTextureSamplerBinding binding = {
            //             .texture = albedo.texture,
            //             .sampler = render_subsystem.GetSampler(ESamplerType::LinearRepeat) // 샘플러는 미리 만들어두고 재사용
            //         };
            //         // Fragment Shader의 0번 슬롯
            //         SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
            //     }
            // }

            // TODO: Mesh Section에 대해서도 렌더링 할 수 있도록 개선
            // info.mesh_id로 AssetManager에서 실제 CPU Mesh를 가져온 후, 아래 코드처럼
            // Draw Sections
            // for (const auto& section : mesh->sections)
            // {
            //     SDL_DrawGPUIndexedPrimitives(render_pass, section.index_count, 1, section.index_start, 0, 0);
            // }

            if (slice.index_count > 0)
            {
                SDL_DrawGPUIndexedPrimitives(pass, slice.index_count, 1, 0, 0, 0);
            }
            else
            {
                const uint32 vertex_count = slice.index_offset / sizeof(Vertex);
                SDL_DrawGPUPrimitives(pass, vertex_count, 1, 0, 0);
            }
        }
    }
    SDL_EndGPURenderPass(pass);
}
}  // namespace se::graphics
