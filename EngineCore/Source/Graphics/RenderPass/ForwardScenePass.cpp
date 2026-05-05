#include "SimpleEngine/Graphics/RenderPass/ForwardScenePass.h"

#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/ECS/EntityPickId.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"
#include "SimpleEngine/Graphics/Manager/PipelineCreateInfo.h"
#include "SimpleEngine/Graphics/Material/SamplerCache.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"
#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"
#include "SimpleEngine/Graphics/View/RenderView.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
using namespace se::math;

SE_BEGIN_REFLECT(ForwardScenePass, meta::Internal)
SE_END_REFLECT(ForwardScenePass)

ForwardScenePass::ForwardScenePass(
    const SceneDrawData& in_draw_data,
    const GpuResourceManager& in_gpu_manager,
    const SamplerCache& in_sampler_cache,
    const RenderView& in_render_view,
    RGTextureHandle in_color_target,
    RGTextureHandle in_depth_target,
    RGTextureHandle in_entity_id_target
)
    : draw_data(in_draw_data)
    , gpu_manager(in_gpu_manager)
    , sampler_cache(in_sampler_cache)
    , render_view(in_render_view)
    , color_target_handle(in_color_target)
    , depth_target_handle(in_depth_target)
    , entity_id_target_handle(in_entity_id_target)
{
}

void ForwardScenePass::Setup(RGSetupContext& context)
{
    // 렌더 타겟 설정
    context.Write(color_target_handle);
    context.Write(depth_target_handle);
    if (entity_id_target_handle.IsValid())
    {
        context.Write(entity_id_target_handle);
    }
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

    SDL_GPUTexture* entity_id_target = entity_id_target_handle.IsValid()
        ? context.GetActualTexture(entity_id_target_handle)
        : nullptr;

    const SDL_GPUColorTargetInfo color_target_info[] = {
        {
            .texture = color_target,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.15f, .g = 0.15f, .b = 0.15f, .a = 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        },
        {
            .texture = entity_id_target,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = { .r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        },
    };
    const uint32 num_color_targets = entity_id_target ? 2u : 1u;

    const SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
        .texture = depth_target,
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
        .clear_stencil = 0,
    };

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, color_target_info, num_color_targets, &depth_stencil_target_info);
    {
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

        // double -> float 변환 헬퍼
        auto to_float4x4 = [](const Matrix4x4& src, Matrix4x4f& dst)
        {
            std::ranges::transform(
                src.data,
                dst.data.begin(),
                [](double v) { return static_cast<float>(v); }
            );
        };

        // Vertex Uniform slot 0: VP 행렬 (per-pass)
        Matrix4x4f vp_f;
        to_float4x4(render_view.view_matrix * render_view.projection_matrix, vp_f);
        SDL_PushGPUVertexUniformData(cmd, 0, &vp_f, sizeof(vp_f));

        // Fragment Uniform slot 0: SceneDataUBO (per-pass)
        struct alignas(16) SceneDataUBO
        {
            Vector3f camera_pos;   float _pad0 = 0.0f; // offset  0~15
            Vector3f light_dir_ws; float _pad1 = 0.0f; // offset 16~31
            Vector3f light_color;  float _pad2 = 0.0f; // offset 32~47
        };
        static_assert(sizeof(SceneDataUBO) == 48, "SceneDataUBO must match HLSL cbuffer layout");

        const SceneDataUBO scene_data = {
            .camera_pos = static_cast<Vector3f>(render_view.camera_pos),
            .light_dir_ws = Vector3f{ 0.0f, 0.0f, -1.0f }, // TODO: RenderView에서 받도록 수정
            .light_color = Vector3f{ 1.0f, 1.0f, 1.0f },
        };
        SDL_PushGPUFragmentUniformData(cmd, 0, &scene_data, sizeof(scene_data));

        // Draw Meshes
        const SDL_GPUGraphicsPipeline* last_pipeline = nullptr;
        for (const DrawCommand& draw_command : draw_data.opaque_commands)
        {
            // CollectDrawData 시점에 GPU 업로드가 완료되지 않았다면 스킵
            if (!draw_command.gpu_buffer)
            {
                continue;
            }

            // Material 정보 조회
            const uint16 slot_idx = draw_command.material_slot_index;
            if (slot_idx == DrawCommand::INVALID_MATERIAL_SLOT)
            {
                continue;
            }

            const FrameMaterialCache& cache = draw_data.material_cache;
            const FrameMaterialCache::MaterialSlot& mat_slot = cache.slots[slot_idx];

            const auto mat_handle_opt = draw_data.pinned_materials.Find(mat_slot.parent_material_id);
            if (!mat_handle_opt)
            {
                continue;
            }
            const Material& material = **mat_handle_opt;

            // 머티리얼 인스턴스 (오버라이드 값 확인용)
            const auto inst_handle_opt = draw_data.pinned_material_instances.Find(draw_command.material_instance_id);
            if (!inst_handle_opt)
            {
                continue;
            }
            const MaterialInstance& instance = **inst_handle_opt;

            // 최종 렌더 상태 결정 (오버라이드 반영)
            const EBlendMode final_blend_mode = instance.GetBlendMode(material);
            const bool final_two_sided = instance.IsTwoSided(material);

            // PSO 결정 및 바인딩
            SDL_GPUGraphicsPipeline* pipeline = [&]
            {
                SDL_GPUVertexBufferDescription vertex_buffer_desc[] = {
                    {
                        .slot = 0,
                        .pitch = sizeof(StaticVertex),
                        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                    },
                };

                SDL_GPUVertexAttribute vertex_attributes[] = {
                    { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = offsetof(StaticVertex, position) },
                    { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = offsetof(StaticVertex, normal) },
                    { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(StaticVertex, tex_coord) },
                    { .location = 3, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = offsetof(StaticVertex, tangent) },
                };

                SDL_GPUColorTargetDescription color_target_desc[] = {
                    {
                        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
                        .blend_state = [&]
                        {
                            SDL_GPUColorTargetBlendState blend_state = {
                                .color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A,
                                .enable_blend = false,
                            };

                            if (final_blend_mode == EBlendMode::Translucent)
                            {
                                blend_state.enable_blend = true;
                                blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
                                blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
                                blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
                                blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
                                blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
                                blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
                            }
                            else if (final_blend_mode == EBlendMode::Additive)
                            {
                                blend_state.enable_blend = true;
                                blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
                                blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
                                blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
                                blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
                                blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
                                blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
                            }

                            return blend_state;
                        }(),
                    },
                    {
                        .format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
                    },
                };

                return context.GetOrCreateGraphicsPipeline({
                    .vertex_shader = material.vertex_shader,
                    .fragment_shader = material.fragment_shader,
                    .vertex_input_state = {
                        .vertex_buffer_descriptions = vertex_buffer_desc,
                        .num_vertex_buffers = std::size(vertex_buffer_desc),
                        .vertex_attributes = vertex_attributes,
                        .num_vertex_attributes = std::size(vertex_attributes),
                    },
                    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                    .rasterizer_state = {
                        .fill_mode = render_view.rendering_mode == ERenderingMode::Wireframe
                                         ? SDL_GPU_FILLMODE_LINE
                                         : SDL_GPU_FILLMODE_FILL,
                        .cull_mode = (render_view.rendering_mode == ERenderingMode::Wireframe || final_two_sided)
                                         ? SDL_GPU_CULLMODE_NONE
                                         : SDL_GPU_CULLMODE_BACK,
                        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
                    },
                    .depth_stencil_state = {
                        .compare_op = SDL_GPU_COMPAREOP_LESS,
                        .enable_depth_test = true,
                        .enable_depth_write = final_blend_mode == EBlendMode::Opaque || final_blend_mode == EBlendMode::Masked,
                        .enable_stencil_test = false,
                    },
                    .target_info = {
                        .color_target_descriptions = color_target_desc,
                        .num_color_targets = num_color_targets,
                        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                        .has_depth_stencil_target = true,
                    },
                });
            }();

            if (pipeline != last_pipeline)
            {
                SDL_BindGPUGraphicsPipeline(pass, pipeline);
                last_pipeline = pipeline;
            }

            // Vertex Buffer 바인딩
            // 셰이더의 Input Slot 0번에 바인딩
            const SDL_GPUBufferBinding vertex_binding = {
                .buffer = draw_command.gpu_buffer,
                .offset = draw_command.vertex_buffer_offset
            };
            SDL_BindGPUVertexBuffers(pass, 0, &vertex_binding, 1);

            if (draw_command.draw_params.index_count > 0)
            {
                // Index Buffer 바인딩
                const SDL_GPUBufferBinding index_binding = {
                    .buffer = draw_command.gpu_buffer,
                    .offset = draw_command.index_buffer_offset
                };
                SDL_BindGPUIndexBuffer(pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            }

            // Vertex Uniform slot 1: Model 행렬 + EntityId (per-object) | TODO: 추후 RTE(Relative To Eye) 방식으로 수정
            struct alignas(16) ObjectUBO
            {
                Matrix4x4f model; // 64 bytes (4 x float4)
                uint32 entity_id; // 4 bytes + 12 bytes padding (= 1 x float4)
            } object_ubo;
            to_float4x4(draw_command.model_matrix, object_ubo.model);
            object_ubo.entity_id = EntityPickId::Encode(draw_command.entity_id).encoded;
            SDL_PushGPUVertexUniformData(cmd, 1, &object_ubo, sizeof(object_ubo));

            // Fragment Uniform slot 1: Material UBO
            SDL_PushGPUFragmentUniformData(
                cmd, 1,
                cache.ubo_arena.Data() + mat_slot.ubo_offset,
                static_cast<uint32>(mat_slot.ubo_size)
            );

            // Fragment Sampler + Texture 바인딩
            Array<SDL_GPUTextureSamplerBinding> tex_bindings;
            for (uint16 b = 0; b < mat_slot.binding_count; ++b)
            {
                const TextureBinding& binding = cache.binding_arena[mat_slot.binding_offset + b];
                const Optional<TextureResource> tex = gpu_manager.GetTexture(binding.texture_id)
                    .OrElse([&]
                    {
                        return gpu_manager.GetTexture(BuiltinAssetIds::White1x1);
                    });

                if (tex.HasValue())
                {
                    tex_bindings.Push({
                        .texture = tex->handle,
                        .sampler = sampler_cache.Get(binding.sampler),
                    });
                }
            }
            if (!tex_bindings.IsEmpty())
            {
                SDL_BindGPUFragmentSamplers(pass, 0, tex_bindings.Data(), static_cast<uint32>(tex_bindings.Len()));
            }

            const IndirectDrawCommand& draw_params = draw_command.draw_params;

            if (draw_params.index_count > 0)
            {
                SDL_DrawGPUIndexedPrimitives(
                    pass,
                    draw_params.index_count,
                    draw_params.instance_count,
                    draw_params.first_index,
                    draw_params.vertex_offset,
                    draw_params.first_instance
                );
            }
            else
            {
                SDL_DrawGPUPrimitives(
                    pass,
                    draw_command.vertex_count,
                    1,
                    static_cast<uint32>(draw_params.vertex_offset),
                    0
                );
            }
        }
    }
    SDL_EndGPURenderPass(pass);
}
} // namespace se
