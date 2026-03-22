// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"

#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Graphics/Manager/PSOManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RGNodeTypes.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphBuilder.h"


namespace se::graphics
{
RGSetupContext::RGSetupContext(RGPassNode& in_pass_node)
    : pass_node_ref(in_pass_node)
{
}

void RGSetupContext::Read(RGTextureHandle handle)
{
    pass_node_ref.read_refs.Push({ .resource_index = handle.index, .version = handle.version });
}

void RGSetupContext::Read(RGBufferHandle handle)
{
    pass_node_ref.read_refs.Push({ .resource_index = handle.index, .version = handle.version });
}

RGTextureHandle RGSetupContext::Write(RGTextureHandle handle)
{
    const uint32 out_version = handle.version + 1;
    pass_node_ref.write_map[handle.index] = out_version;
    return RGTextureHandle{ .index = handle.index, .version = out_version };
}

RGBufferHandle RGSetupContext::Write(RGBufferHandle handle)
{
    const uint32 out_version = handle.version + 1;
    pass_node_ref.write_map[handle.index] = out_version;
    return RGBufferHandle{ .index = handle.index, .version = out_version };
}

RGExecutionContext::RGExecutionContext(
    SDL_GPUCommandBuffer* in_cmd,
    PSOManager& in_pso_manager,
    const RenderGraphBuilder& in_builder
)
    : command_buffer(in_cmd)
    , pso_manager(in_pso_manager)
    , builder_ref(in_builder)
{
}

SDL_GPUTexture* RGExecutionContext::GetActualTexture(RGTextureHandle handle) const
{
    if (handle.index < builder_ref.resource_nodes.Len())
    {
        RGResourceBase* raw_ptr = builder_ref.resource_nodes[handle.index].resource.get();
        if (const RGTextureBase* resource = Cast<RGTextureBase>(raw_ptr))
        {
            return resource->GetActualTexture();
        }
    }
    return nullptr;
}

SDL_GPUBuffer* RGExecutionContext::GetActualBuffer(RGBufferHandle handle) const
{
    if (handle.index < builder_ref.resource_nodes.Len())
    {
        RGResourceBase* raw_ptr = builder_ref.resource_nodes[handle.index].resource.get();
        if (const RGBufferBase* resource = Cast<RGBufferBase>(raw_ptr))
        {
            return resource->GetActualBuffer();
        }
    }
    return nullptr;
}

SDL_GPUGraphicsPipeline* RGExecutionContext::GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
{
    return pso_manager.GetOrCreateGraphicsPipeline(create_info);
}

SDL_GPUComputePipeline* RGExecutionContext::GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info)
{
    return pso_manager.GetOrCreateComputePipeline(create_info);
}
} // namespace se::graphics