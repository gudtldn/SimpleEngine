// ReSharper disable CppMemberFunctionMayBeConst
module SimpleEngine.Rendering;
import :RenderGraph;

import <cassert>;


namespace se::rendering::render_graph
{
void RenderGraph::Compile()
{
    for (RGPassNode& pass_node : pass_nodes)
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
    for (const auto& [_, resource] : resource_nodes)
    {
        resource->Realize(device);
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
    for (const auto& [_, resource] : resource_nodes)
    {
        resource->Unrealize(device);
    }

    pass_nodes.clear();
    resource_nodes.clear();
    compiled_passes.clear();
}

RGResourceHandle RenderGraph::ImportTexture(const StringName& name, SDL_GPUTexture* texture)
{
    assert(!FindResource(name) && "A resource with the same name already exists.");

    auto texture_resource = std::make_unique<RGExternalTexture>(texture);
    return RegisterResource({
        .name = name,
        .resource = std::move(texture_resource),
    });
}

RGResourceHandle RenderGraph::ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer)
{
    assert(!FindResource(name) && "A resource with the same name already exists.");

    auto buffer_resource = std::make_unique<RGExternalBuffer>(buffer);
    return RegisterResource({
        .name = name,
        .resource = std::move(buffer_resource),
    });
}

Optional<RGResourceHandle> RenderGraph::FindResource(const StringName& name) const
{
    for (auto [n, pass_node] : resource_nodes | std::views::enumerate)
    {
        if (pass_node.name == name)
        {
            return RGResourceHandle{ static_cast<size_t>(n) };
        }
    }
    return std::nullopt;
}

SDL_GPUTexture* RenderGraph::GetActualTexture(RGResourceHandle handle) const
{
    if (handle.index < resource_nodes.size())
    {
        IRGResource* raw_ptr = resource_nodes[handle.index].resource.get();
        if (const RGTransientTexture* resource = dynamic_cast<RGTransientTexture*>(raw_ptr))
        {
            return resource->actual_texture;
        }
        if (const RGExternalTexture* resource = dynamic_cast<RGExternalTexture*>(raw_ptr))
        {
            return resource->actual_texture;
        }
    }
    return nullptr;
}

SDL_GPUBuffer* RenderGraph::GetActualBuffer(RGResourceHandle handle) const
{
    if (handle.index < resource_nodes.size())
    {
        IRGResource* raw_ptr = resource_nodes[handle.index].resource.get();
        if (const RGTransientBuffer* resource = dynamic_cast<RGTransientBuffer*>(raw_ptr))
        {
            return resource->actual_buffer;
        }
        if (const RGExternalBuffer* resource = dynamic_cast<RGExternalBuffer*>(raw_ptr))
        {
            return resource->actual_buffer;
        }
    }
    return nullptr;
}

RGResourceHandle RenderGraph::RegisterResource(RGResourceNode&& node)
{
    const size_t index = resource_nodes.size();
    resource_nodes.push_back(std::move(node));
    return { .index = index };
}

RGResourceHandle RenderGraphBuilder::CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& description)
{
    assert(!graph_ref.FindResource(name) && "A resource with the same name already exists.");

    std::unique_ptr<RGTransientTexture> texture_resource = std::make_unique<RGTransientTexture>();
    texture_resource->description = description;

    return graph_ref.RegisterResource({
        .name = name,
        .resource = std::move(texture_resource),
    });
}

RGResourceHandle RenderGraphBuilder::CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& description)
{
    assert(!graph_ref.FindResource(name) && "A resource with the same name already exists.");

    std::unique_ptr<RGTransientBuffer> buffer_resource = std::make_unique<RGTransientBuffer>();
    buffer_resource->description = description;

    return graph_ref.RegisterResource({
        .name = name,
        .resource = std::move(buffer_resource),
    });
}

RGResourceHandle RenderGraphBuilder::ImportTexture(const StringName& name, SDL_GPUTexture* texture)
{
    return graph_ref.ImportTexture(name, texture);
}

RGResourceHandle RenderGraphBuilder::ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer)
{
    return graph_ref.ImportBuffer(name, buffer);
}

Optional<RGResourceHandle> RenderGraphBuilder::FindResource(const StringName& name) const
{
    return graph_ref.FindResource(name);
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
