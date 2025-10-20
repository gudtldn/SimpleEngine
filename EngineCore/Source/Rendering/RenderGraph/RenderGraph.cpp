// ReSharper disable CppMemberFunctionMayBeConst
#include "Rendering/RenderGraph/RenderGraph.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <utility>

#include "Core/Logging/Logging.h"
#include "Rendering/Manager/PSOManager.h"
#include "tracy/Tracy.hpp"


namespace se::rendering
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
    ZoneScoped;

    // 1. Setup 단계
    // 각 패스의 Setup을 호출하여 먼저 이번 프레임에 사용할 리소스를 체크
    for (RGPassNode& pass_node : pass_nodes)
    {
        RenderGraphBuilder builder(*this, pass_node);
        pass_node.pass_object->Setup(builder);
    }

    for (RGPassNode& pass_node : pass_nodes)
    {
        // 이 패스가 사용하는 리소스에 writer_pass 정보를 추가
        for (const RGResourceHandle write_handle : pass_node.writes)
        {
            if constexpr (SE_DEBUG_BUILD)
            {
                // 리소스가 존재하는지 확인
                if (!(write_handle && write_handle.index < resource_nodes.size()))
                {
                    const IRenderPass* const pass_object = pass_node.pass_object.get();
                    ConsoleLog(
                        ELogLevel::Error,
                        u8"Invalid resource handle. Check the {}::Setup() logic",
                        typeid(*pass_object).name()
                    );
                    assert(false && "Invalid resource handle.");
                }
            }

            RGResourceNode& resource_node = resource_nodes[write_handle.index];

            // 리소스가 만들어졌는지 확인
            if constexpr (SE_DEBUG_BUILD)
            {
                if (!resource_node.resource)
                {
                    const IRenderPass* const pass_object = pass_node.pass_object.get();
                    ConsoleLog(
                        ELogLevel::Error,
                        u8"Resource {} is not initialized. Check the {}::Setup() logic",
                        resource_node.name.ToString(),
                        typeid(*pass_object).name()
                    );
                    assert(false && "Resource is not initialized.");
                }

                // 리소스가 이미 다른 패스에서 쓰고 있는지 확인
                if (resource_node.writer && resource_node.writer != &pass_node)
                {
                    const IRenderPass* const existing_writer_pass = resource_node.writer->pass_object.get();
                    const IRenderPass* const pass_object = pass_node.pass_object.get();
                    // 각 리소스는 한 프레임에 하나의 패스에서만 쓰여야함
                    ConsoleLog(
                        ELogLevel::Error,
                        u8"Multiple write passes detected for the same resource.\n"
                        u8"  - Resource Name: {}\n"
                        u8"  - Existing Writer Pass: {}\n"
                        u8"  - Conflicting Writer Pass: {}",
                        resource_node.name.ToString(),
                        typeid(*existing_writer_pass).name(),
                        typeid(*pass_object).name()
                    );
                    assert(false && "A resource can only be written by a single pass per frame.");
                }
            }
            resource_node.writer = &pass_node;
        }
    }

    // 2. Pass Culling 단계
    // 사용되고 있는 리소스를 추적해서 패스를 활성화로 만들기
    queue<RGResourceHandle> active_resources;
    for (const auto& [n, resource_node] : resource_nodes | std::views::enumerate)
    {
        IRGResource* resource = resource_node.resource.get();
        if (
            dynamic_cast<RGExternalTexture*>(resource)
            || dynamic_cast<RGExternalBuffer*>(resource)
        )
        {
            active_resources.push({ static_cast<size_t>(n) });
        }
    }

    // 역방향으로 그래프를 순회하며 활성 패스를 찾아냄
    while (!active_resources.empty())
    {
        const auto [active_handle_idx] = active_resources.front();
        active_resources.pop();

        if (const RGPassNode* writer_pass = resource_nodes[active_handle_idx].writer)
        {
            const size_t pass_index = writer_pass - &pass_nodes[0];
            RGPassNode& pass_to_activate = pass_nodes[pass_index];

            // 이미 활성화 상태면 건너뛰기
            if (!pass_to_activate.culled)
            {
                continue;
            }

            // 패스를 활성화 상태로 변경
            pass_to_activate.culled = false;

            // pass_to_activate가 사용하고 있는 Read 리소스를 active_resources에 추가
            for (const auto& [read_handle_idx] : pass_to_activate.reads)
            {
                active_resources.push({ static_cast<size_t>(read_handle_idx) });
            }
        }
    }

    // 3. Pass간 위상정렬 수행
    unordered_map<const RGPassNode*, uint32> in_degrees;

    // 진입 차수 계산 (각 패스가 몇 개의 다른 패스에 의존하는지 계산)
    for (const RGPassNode& pass_node : pass_nodes)
    {
        if (pass_node.culled)
        {
            continue;
        }

        for (const auto [read_handle_idx] : pass_node.reads)
        {
            const RGResourceNode& resource_node = resource_nodes[read_handle_idx];
            if (
                resource_node.writer
                && !resource_node.writer->culled
            )
            {
                // pass_node가 읽고 있는 리소스를 쓰는 다른 패스가 있다면, 의존성 +1
                ++in_degrees[&pass_node];
            }
        }
    }

    // 진입 차수가 0인 패스(누구에게도 의존하지 않는 패스)들을 큐에 추가
    queue<const RGPassNode*> ready_queue;
    for (const RGPassNode& pass_node : pass_nodes)
    {
        if (!pass_node.culled && in_degrees[&pass_node] == 0)
        {
            ready_queue.push(&pass_node);
        }
    }

    // 위상 정렬 수행
    while (!ready_queue.empty())
    {
        const RGPassNode* pass_node = ready_queue.front();
        ready_queue.pop();

        compiled_passes.push_back(pass_node);

        // 현재 패스가 쓴 리소스를 읽는 다른 패스들의 의존성을 해결
        for (const RGResourceHandle& write_handle : pass_node->writes)
        {
            // 이 리소스를 읽는 모든 패스를 찾아 진입 차수를 1 감소
            for (const RGPassNode& potential_reader_pass : pass_nodes)
            {
                if (potential_reader_pass.culled)
                {
                    continue;
                }

                for (const RGResourceHandle& read_handle : potential_reader_pass.reads)
                {
                    if (read_handle == write_handle)
                    {
                        --in_degrees[&potential_reader_pass];
                        if (in_degrees[&potential_reader_pass] == 0)
                        {
                            ready_queue.push(&potential_reader_pass);
                        }
                    }
                }
            }
        }
    }

    // 순환 의존성 확인
    const size_t active_pass_count = std::ranges::count_if(pass_nodes, [](const RGPassNode& pass_node)
    {
        return !pass_node.culled;
    });

    if (compiled_passes.size() != active_pass_count)
    {
        vector<const RGPassNode*> circular_pass_node;
        for (const auto& [pass_node, degree] : in_degrees)
        {
            if (degree > 0)
            {
                circular_pass_node.push_back(pass_node);
            }
        }

        ConsoleLog(ELogLevel::Fatal, u8"A cycle was detected in the render graph!");
        ConsoleLog(ELogLevel::Fatal, u8"The following subsystems are involved in a circular dependency:");
        for (const RGPassNode* node : circular_pass_node)
        {
            const IRenderPass* const pass_object = node->pass_object.get();
            ConsoleLog(ELogLevel::Fatal, u8"- {}", typeid(*pass_object).name());
        }

        assert(false && "A cycle was detected in the render graph!");
        return;
    }

    // TODO: 리소스 생명주기 계산
}

void RenderGraph::Execute(SDL_GPUCommandBuffer* cmd, PSOManager& pso_manager)
{
    ZoneScoped;

    // TODO: 컴파일 단계에서 리소스 lifecycle 체크해서 필요한 시점에만 리소스를 할당하고 재사용하게끔 수정

    for (const RGPassNode* pass_node : compiled_passes)
    {
        for (const auto [write_handle_idx] : pass_node->writes)
        {
            resource_nodes[write_handle_idx].resource->Realize(resource_pool);
        }
        pass_node->pass_object->Execute({ cmd, pso_manager, *this });
    }
}

void RenderGraph::Clear()
{
    for (const RGPassNode* pass_node : compiled_passes)
    {
        for (const auto& [write_handle_idx] : pass_node->writes)
        {
            resource_nodes[write_handle_idx].resource->Unrealize(resource_pool);
        }
    }

    pass_nodes.clear();
    resource_nodes.clear();
    resource_name_map.clear();
    compiled_passes.clear();
}

RGResourceHandle RenderGraph::ImportTexture(const StringName& name, SDL_GPUTexture* texture)
{
    const RGResourceHandle handle = GetResourceHandleByName(name);
    RGResourceNode& node = resource_nodes[handle.index];

    assert(!node.resource && "A resource with the same name already exists.");

    node.resource = std::make_unique<RGExternalTexture>(texture);
    return handle;
}

RGResourceHandle RenderGraph::ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer)
{
    const RGResourceHandle handle = GetResourceHandleByName(name);
    RGResourceNode& node = resource_nodes[handle.index];

    assert(!node.resource && "A resource with the same name already exists.");

    node.resource = std::make_unique<RGExternalBuffer>(buffer);
    return handle;
}

RGResourceHandle RenderGraph::GetResourceHandleByName(const StringName& name)
{
    if (const auto it = resource_name_map.find(name); it != resource_name_map.end())
    {
        return it->second;
    }

    const RGResourceHandle new_handle = RegisterResource({ .name = name });
    resource_name_map[name] = new_handle;

    return new_handle;
}

RGResourceHandle RenderGraph::RegisterResource(RGResourceNode&& node)
{
    const size_t index = resource_nodes.size();
    resource_nodes.push_back(std::move(node));
    return { .index = index };
}


RGResourceHandle RenderGraphBuilder::GetResourceHandleByName(const StringName& name) const
{
    return graph_ref.GetResourceHandleByName(name);
}

RGResourceHandle RenderGraphBuilder::CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& description)
{
    const RGResourceHandle handle = GetResourceHandleByName(name);
    RGResourceNode& node = graph_ref.resource_nodes[handle.index];

    assert(!node.resource && "A resource with the same name already exists.");

    std::unique_ptr<RGTransientTexture> texture_resource = std::make_unique<RGTransientTexture>();
    texture_resource->description = description;

    node.resource = std::move(texture_resource);
    Write(handle);

    return handle;
}

RGResourceHandle RenderGraphBuilder::CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& description)
{
    const RGResourceHandle handle = GetResourceHandleByName(name);
    RGResourceNode& node = graph_ref.resource_nodes[handle.index];

    assert(!node.resource && "A resource with the same name already exists.");

    std::unique_ptr<RGTransientBuffer> buffer_resource = std::make_unique<RGTransientBuffer>();
    buffer_resource->description = description;

    node.resource = std::move(buffer_resource);
    Write(handle);

    return handle;
}

void RenderGraphBuilder::Read(RGResourceHandle handle)
{
    pass_node_ref.reads.emplace(handle);
}

void RenderGraphBuilder::Write(RGResourceHandle handle)
{
    pass_node_ref.writes.emplace(handle);
}


SDL_GPUTexture* RGExecutionContext::GetActualTexture(RGResourceHandle handle) const
{
    if (handle.index < graph_ref.resource_nodes.size())
    {
        IRGResource* raw_ptr = graph_ref.resource_nodes[handle.index].resource.get();
        if (const IRGTexture* resource = dynamic_cast<IRGTexture*>(raw_ptr)) // TODO: 나중에 dynamic_cast를 대체하는 방향으로 수정
        {
            return resource->GetActualTexture();
        }
    }
    return nullptr;
}

SDL_GPUBuffer* RGExecutionContext::GetActualBuffer(RGResourceHandle handle) const
{
    if (handle.index < graph_ref.resource_nodes.size())
    {
        IRGResource* raw_ptr = graph_ref.resource_nodes[handle.index].resource.get();
        if (const IRGBuffer* resource = dynamic_cast<IRGBuffer*>(raw_ptr)) // TODO: 나중에 dynamic_cast를 대체하는 방향으로 수정
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
}
