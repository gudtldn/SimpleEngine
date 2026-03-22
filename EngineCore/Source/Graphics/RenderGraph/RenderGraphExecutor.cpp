// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphExecutor.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Graphics/Manager/PSOManager.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "SimpleEngine/Utility/Debug.h"

#include "tracy/Tracy.hpp"

#include <algorithm>
#include <ranges>


namespace se::graphics
{
RenderGraphExecutor::RenderGraphExecutor(RenderDevice& in_render_device)
    : render_device(&in_render_device)
    , resource_pool(in_render_device)
{
}

RenderGraphExecutor::~RenderGraphExecutor()
{
}

void RenderGraphExecutor::Execute(
    RenderGraphBuilder& builder,
    SDL_GPUCommandBuffer* cmd,
    PSOManager& pso_manager
)
{
    ZoneScoped;

    Compile(builder);

    for (usize pass_index = 0; pass_index < compiled_passes.Len(); ++pass_index)
    {
        const RGPassNode* pass_node = compiled_passes[pass_index];

        ZoneScoped;
        SE_DEBUG_EXPRESSION({
            String zone_name = String::Format("RenderGraphExecutor::Execute - {}::Execute", pass_node->name);
            ZoneName(zone_name.CStr(), zone_name.ByteLen());
        })

        // 이 패스에서 생명주기가 시작되는 리소스를 할당합니다.
        for (const usize resource_idx : resources_to_realize[pass_index])
        {
            builder.resource_nodes[resource_idx].resource->Realize(resource_pool);
        }

        RGExecutionContext context{ cmd, pso_manager, builder };
        pass_node->pass_object->Execute(context);

        // 이 패스에서 생명주기가 끝나는 리소스를 해제합니다.
        for (const usize resource_idx : resources_to_unrealize[pass_index])
        {
            builder.resource_nodes[resource_idx].resource->Unrealize(resource_pool);
        }
    }

    // Execute()중 Unrealize되지 못한 리소스 정리
    for (RGResourceNode& node : builder.resource_nodes)
    {
        if (node.resource)
        {
            node.resource->Unrealize(resource_pool);
        }
    }

    builder.Clear();
}

void RenderGraphExecutor::UpdateResourcePool()
{
    static constexpr uint32 MAX_IDLE_FRAMES = 3;
    resource_pool.IncrementIdleCounters();
    resource_pool.Trim(MAX_IDLE_FRAMES);
}

void RenderGraphExecutor::Compile(RenderGraphBuilder& builder)
{
    ZoneScoped;

    // 매 컴파일마다 초기화
    compiled_passes.Clear();
    resources_to_realize.Clear();
    resources_to_unrealize.Clear();

    // 1. Setup 단계: 각 패스에서 Read/Write 선언을 수집
    for (RGPassNode& pass_node : builder.pass_nodes)
    {
        ZoneScoped;
        SE_DEBUG_EXPRESSION({
            String zone_name = String::Format("RenderGraphExecutor::Compile - {}::Setup", pass_node.name.ToString());
            ZoneName(zone_name.CStr(), zone_name.ByteLen());
        });
        RGSetupContext setup_ctx(pass_node);
        pass_node.pass_object->Setup(setup_ctx);
    }

    // 2. Writer 등록: 각 패스의 write_indices를 기반으로 리소스의 writer_pass_index를 설정
    for (const auto [pass_index, pass_node] : builder.pass_nodes | std::views::enumerate)
    {
        const uint32 idx = static_cast<uint32>(pass_index);
        for (const uint32 write_idx : pass_node.write_indices)
        {
#if SE_BUILD_DEBUG
            if (write_idx >= builder.resource_nodes.Len())
            {
                ConsoleLog(
                    ELogLevel::Error,
                    "Invalid resource index {} in {}::Setup()",
                    write_idx,
                    pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Invalid resource index.");
                continue;
            }

            RGResourceNode& resource_node = builder.resource_nodes[write_idx];
            if (!resource_node.resource)
            {
                ConsoleLog(
                    ELogLevel::Error,
                    "Resource {} is not initialized. Check {}::Setup()",
                    resource_node.name.ToString(),
                    pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Resource is not initialized.");
                continue;
            }

            if (
                resource_node.writer_pass_index != INVALID_PASS_INDEX
                && resource_node.writer_pass_index != idx
            )
            {
                const StringName& existing_writer = builder.pass_nodes[resource_node.writer_pass_index].name;
                ConsoleLog(
                    ELogLevel::Error,
                    "Multiple write passes for resource '{}':\n"
                    "  - Existing writer: {}\n"
                    "  - Conflicting writer: {}",
                    resource_node.name.ToString(),
                    existing_writer.ToString(),
                    pass_node.name.ToString()
                );
                SE_ASSERT(false, "A resource can only be written by a single pass per frame.");
                continue;
            }
#endif
            builder.resource_nodes[write_idx].writer_pass_index = idx;
        }
    }

    // 3. Pass Culling: 외부 리소스에서 역방향으로 활성 패스를 조회
    Queue<uint32> active_resource_indices;
    for (const auto [n, resource_node] : builder.resource_nodes | std::views::enumerate)
    {
        const RGResourceBase* resource = resource_node.resource.get();
        if (IsA<RGExternalTexture>(resource) || IsA<RGExternalBuffer>(resource))
        {
            active_resource_indices.Push(static_cast<uint32>(n));
        }
    }

    while (Optional active_idx_opt = active_resource_indices.Pop())
    {
        const uint32 active_idx = *active_idx_opt;
        const uint32 writer_idx = builder.resource_nodes[active_idx].writer_pass_index;
        if (writer_idx == INVALID_PASS_INDEX)
        {
            continue;
        }

        RGPassNode& pass_to_activate = builder.pass_nodes[writer_idx];
        if (!pass_to_activate.culled)
        {
            continue;
        }

        pass_to_activate.culled = false;

        for (const uint32 read_idx : pass_to_activate.read_indices)
        {
            active_resource_indices.Push(read_idx);
        }
    }

    // 4. 위상정렬 (Kahn's algorithm)
    HashMap<const RGPassNode*, uint32> in_degrees;

    for (const RGPassNode& pass_node : builder.pass_nodes)
    {
        if (pass_node.culled)
        {
            continue;
        }

        for (const uint32 read_idx : pass_node.read_indices)
        {
            const uint32 writer_idx = builder.resource_nodes[read_idx].writer_pass_index;
            if (writer_idx != INVALID_PASS_INDEX && !builder.pass_nodes[writer_idx].culled)
            {
                ++in_degrees[&pass_node];
            }
        }
    }

    Queue<const RGPassNode*> ready_queue;
    for (const RGPassNode& pass_node : builder.pass_nodes)
    {
        if (!pass_node.culled && in_degrees[&pass_node] == 0)
        {
            ready_queue.Push(&pass_node);
        }
    }

    while (Optional pass_opt = ready_queue.Pop())
    {
        const RGPassNode* pass_node = *pass_opt;
        compiled_passes.Push(pass_node);

        for (const uint32 write_idx : pass_node->write_indices)
        {
            for (const RGPassNode& potential_reader : builder.pass_nodes)
            {
                if (potential_reader.culled)
                {
                    continue;
                }

                for (const uint32 read_idx : potential_reader.read_indices)
                {
                    if (read_idx == write_idx)
                    {
                        --in_degrees[&potential_reader];
                        if (in_degrees[&potential_reader] == 0)
                        {
                            ready_queue.Push(&potential_reader);
                        }
                    }
                }
            }
        }
    }

    // 순환 의존성 확인
    const usize active_pass_count = std::ranges::count_if(
        builder.pass_nodes,
        [](const RGPassNode& p) { return !p.culled; }
    );

    if (compiled_passes.Len() != active_pass_count)
    {
        ConsoleLog(ELogLevel::Fatal, "A cycle was detected in the render graph!");
        for (const auto& [pass_node, degree] : in_degrees)
        {
            if (degree > 0)
            {
                ConsoleLog(ELogLevel::Fatal, "  - {}", pass_node->name.ToString());
            }
        }
        SE_ASSERT(false, "A cycle was detected in the render graph!");
        return;
    }

    // 5. 리소스 생명주기 분석
    for (const auto [pass_idx, pass_node] : compiled_passes | std::views::enumerate)
    {
        const uint32 idx = static_cast<uint32>(pass_idx);

        for (const uint32 read_idx : pass_node->read_indices)
        {
            RGResourceNode& node = builder.resource_nodes[read_idx];
            node.first_user_pass_index = std::min(node.first_user_pass_index, idx);
            node.last_user_pass_index  = std::max(node.last_user_pass_index, idx);
        }
        for (const uint32 write_idx : pass_node->write_indices)
        {
            RGResourceNode& node = builder.resource_nodes[write_idx];
            node.first_user_pass_index = std::min(node.first_user_pass_index, idx);
            node.last_user_pass_index  = std::max(node.last_user_pass_index, idx);
        }
    }

    // 6. 리소스 할당/해제 스케줄 구축
    const usize pass_count = compiled_passes.Len();
    resources_to_realize.Resize(pass_count);
    resources_to_unrealize.Resize(pass_count);

    for (const auto [idx, node] : builder.resource_nodes | std::views::enumerate)
    {
        if (node.first_user_pass_index <= node.last_user_pass_index)
        {
            resources_to_realize[node.first_user_pass_index].Push(static_cast<usize>(idx));
            resources_to_unrealize[node.last_user_pass_index].Push(static_cast<usize>(idx));
        }
    }
}
} // namespace se::graphics