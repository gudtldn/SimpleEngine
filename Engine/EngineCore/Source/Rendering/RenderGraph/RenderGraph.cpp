// ReSharper disable CppMemberFunctionMayBeConst
module SE.Rendering;
import :RenderGraph;

import <cassert>;


namespace se::rendering::render_graph
{
RenderGraph::RenderGraph(SDL_GPUDevice* in_device)
    : device(in_device)
    , resource_pool(in_device)
{
}

RenderGraph::~RenderGraph()
{
    Clear();
}

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
}

void RenderGraph::Execute(SDL_GPUCommandBuffer* cmd, manager::PSOManager& pso_manager)
{
    resource_pool.BeginFrame();

    // TODO: 컴파일 단계에서 리소스 lifecycle 체크해서 필요한 시점에만 리소스를 할당하고 재사용하게끔 수정
    // 일단 모든 리소스를 미리 생성
    for (const auto& [_, resource] : resource_nodes)
    {
        resource->Realize(resource_pool);
    }

    for (const RGPassNode* pass_node : compiled_passes)
    {
        pass_node->pass_object->Execute({ cmd, pso_manager, *this });
    }
}

void RenderGraph::Clear()
{
    for (const auto& [_, resource] : resource_nodes)
    {
        resource->Unrealize();
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
            return RGResourceHandle{ .index = static_cast<size_t>(n) };
        }
    }
    return std::nullopt;
}

SDL_GPUTexture* RenderGraph::GetActualTexture(RGResourceHandle handle) const
{
    if (handle.index < resource_nodes.size())
    {
        IRGResource* raw_ptr = resource_nodes[handle.index].resource.get();
        if (const IRGTexture* resource = dynamic_cast<IRGTexture*>(raw_ptr)) // TODO: 나중에 dynamic_cast를 대체하는 방향으로 수정
        {
            return resource->GetActualTexture();
        }
    }
    return nullptr;
}

SDL_GPUBuffer* RenderGraph::GetActualBuffer(RGResourceHandle handle) const
{
    if (handle.index < resource_nodes.size())
    {
        IRGResource* raw_ptr = resource_nodes[handle.index].resource.get();
        if (const IRGBuffer* resource = dynamic_cast<IRGBuffer*>(raw_ptr)) // TODO: 나중에 dynamic_cast를 대체하는 방향으로 수정
        {
            return resource->GetActualBuffer();
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

SDL_GPUGraphicsPipeline* RGExecutionContext::GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info)
{
    return pso_manager.GetOrCreateGraphicsPipeline(create_info);
}

SDL_GPUComputePipeline* RGExecutionContext::GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info)
{
    return pso_manager.GetOrCreateComputePipeline(create_info);
}
}
