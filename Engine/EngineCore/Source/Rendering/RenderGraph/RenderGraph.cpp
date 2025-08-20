// ReSharper disable CppMemberFunctionMayBeConst
module SimpleEngine.Rendering;
import :RenderGraph;

import <cassert>;


namespace se::rendering::render_graph
{
void RenderGraph::Compile()
{
    for (auto& pass_node : pass_nodes)
    {
        RenderGraphBuilder builder(*this, pass_node);
        pass_node.pass_object->Setup(builder);
    }

    // TODO: 사용되지 않는 리소스나 패스를 제거(Culling)하고, reads/writes 정보로 위상정렬 수행
    compiled_passes.clear();
    for (const RGPassNode& pass_node : pass_nodes)
    {
        compiled_passes.push_back(&pass_node);
    }

    // TODO: 리소스 lifecycle 체크해서 필요한 시점에만 리소스를 할당하고 재사용하게끔 수정
    // 일단 모든 리소스를 미리 생성
    for (RGResourceNode& resource_node : resource_nodes)
    {
        switch (resource_node.type)
        {
        case ERGResourceType::Texture:
        {
            RGTexture* resource_ptr = static_cast<RGTexture*>(resource_node.resource.get());
            resource_ptr->actual_texture = SDL_CreateGPUTexture(device, &resource_ptr->description);
            break;
        }
        case ERGResourceType::Buffer:
        {
            RGBuffer* resource_ptr = static_cast<RGBuffer*>(resource_node.resource.get());
            resource_ptr->actual_buffer = SDL_CreateGPUBuffer(device, &resource_ptr->description);
            break;
        }
        default:
            assert(false);
        }
    }
}

void RenderGraph::Execute(SDL_GPUCommandBuffer* cmd)
{
    for (const RGPassNode* pass_node : compiled_passes)
    {
        pass_node->pass_object->Execute({ cmd, *this });
    }
}

void RenderGraph::Clear()
{
    for (RGResourceNode& resource_node : resource_nodes)
    {
        switch (resource_node.type)
        {
        case ERGResourceType::Texture:
        {
            RGTexture* resource_ptr = static_cast<RGTexture*>(resource_node.resource.get());
            SDL_ReleaseGPUTexture(device, resource_ptr->actual_texture);
            resource_ptr->actual_texture = nullptr;
            break;
        }
        case ERGResourceType::Buffer:
        {
            RGBuffer* resource_ptr = static_cast<RGBuffer*>(resource_node.resource.get());
            SDL_ReleaseGPUBuffer(device, resource_ptr->actual_buffer);
            resource_ptr->actual_buffer = nullptr;
            break;
        }
        default:
            assert(false);
        }
    }

    pass_nodes.clear();
    resource_nodes.clear();
    compiled_passes.clear();
}

SDL_GPUTexture* RenderGraph::GetActualTexture(RGResourceHandle handle) const
{
    if (handle && handle.index < resource_nodes.size())
    {
        if (resource_nodes[handle.index].type == ERGResourceType::Texture)
        {
            return static_cast<RGTexture*>(resource_nodes[handle.index].resource.get())->actual_texture;
        }
    }
    return nullptr;
}

SDL_GPUBuffer* RenderGraph::GetActualBuffer(RGResourceHandle handle) const
{
    if (handle && handle.index < resource_nodes.size())
    {
        if (resource_nodes[handle.index].type == ERGResourceType::Buffer)
        {
            return static_cast<RGBuffer*>(resource_nodes[handle.index].resource.get())->actual_buffer;
        }
    }
    return nullptr;
}

RGResourceHandle RenderGraph::RegisterResource(RGResourceNode&& node)
{
    const size_t index = resource_nodes.size();
    resource_nodes.push_back(std::move(node));
    return { index };
}

RGResourceHandle RenderGraphBuilder::CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& description)
{
    std::unique_ptr<RGTexture> texture_resource = std::make_unique<RGTexture>();
    texture_resource->description = description;
    return graph_ref.RegisterResource({
        .name = name,
        .resource = std::move(texture_resource),
        .type = ERGResourceType::Texture,
    });
}

RGResourceHandle RenderGraphBuilder::CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& description)
{
    std::unique_ptr<RGBuffer> buffer_resource = std::make_unique<RGBuffer>();
    buffer_resource->description = description;
    return graph_ref.RegisterResource({
        .name = name,
        .resource = std::move(buffer_resource),
        .type = ERGResourceType::Buffer,
    });
}

void RenderGraphBuilder::Read(RGResourceHandle handle)
{
    pass_node_ref.reads.push_back(handle);
}

void RenderGraphBuilder::Write(RGResourceHandle handle)
{
    pass_node_ref.writes.push_back(handle);
}

SDL_GPUTexture* RGExecutionContext::GetActualTexture(RGResourceHandle handle) const
{
    return graph_ref.GetActualTexture(handle);
}

SDL_GPUBuffer* RGExecutionContext::GetActualBuffer(RGResourceHandle handle) const
{
    return graph_ref.GetActualBuffer(handle);
}
}
