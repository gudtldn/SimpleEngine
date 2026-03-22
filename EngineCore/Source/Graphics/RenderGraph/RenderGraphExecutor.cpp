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

    // 2. Writer 등록: 각 패스의 write_map을 기반으로 리소스의 version_to_writer를 설정
    for (const auto [pass_index, pass_node] : builder.pass_nodes | std::views::enumerate)
    {
        const uint32 idx = static_cast<uint32>(pass_index);
        for (const auto& [res_idx, out_ver] : pass_node.write_map)
        {
#if SE_BUILD_DEBUG
            if (res_idx >= builder.resource_nodes.Len())
            {
                ConsoleLog(
                    ELogLevel::Error,
                    "Invalid resource index {} in {}::Setup()",
                    res_idx,
                    pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Invalid resource index.");
                continue;
            }

            RGResourceNode& resource_node = builder.resource_nodes[res_idx];
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

            if (const Optional existing = resource_node.version_to_writer.Find(out_ver))
            {
                if (*existing != idx)
                {
                    const StringName& existing_writer = builder.pass_nodes[*existing].name;
                    ConsoleLog(
                        ELogLevel::Error,
                        "Multiple write passes for resource '{}' version {}:\n"
                        "  - Existing writer: {}\n"
                        "  - Conflicting writer: {}",
                        resource_node.name.ToString(),
                        out_ver,
                        existing_writer.ToString(),
                        pass_node.name.ToString()
                    );
                    SE_ASSERT(false, "A resource version can only be written by a single pass per frame.");
                    continue;
                }
            }
#endif
            builder.resource_nodes[res_idx].version_to_writer[out_ver] = idx;
        }
    }

    // 3. Pass Culling: 외부 리소스에서 역방향으로 활성 패스를 조회
    //    외부 리소스의 최신 버전을 생성한 패스부터 역탐색합니다.
    struct ActiveResourceEntry { uint32 index; uint32 version; };
    Queue<ActiveResourceEntry> active_resource_queue;

    for (const auto [n, resource_node] : builder.resource_nodes | std::views::enumerate)
    {
        const RGResourceBase* resource = resource_node.resource.get();
        if (!IsA<RGExternalTexture>(resource) && !IsA<RGExternalBuffer>(resource))
        {
            continue;
        }

        // 외부 리소스의 최신 버전(가장 높은 버전)을 활성화 트리거로 사용
        uint32 max_version = 0;
        bool has_writer = false;
        for (const auto& [ver, writer_idx] : resource_node.version_to_writer)
        {
            if (!has_writer || ver > max_version)
            {
                max_version = ver;
                has_writer = true;
            }
        }

        if (has_writer)
        {
            active_resource_queue.Push({
                .index = static_cast<uint32>(n),
                .version = max_version,
            });
        }
    }

    while (Optional<ActiveResourceEntry> active_opt = active_resource_queue.Pop())
    {
        const auto& [active_idx, active_version] = *active_opt;
        const Optional writer_opt = builder.resource_nodes[active_idx].version_to_writer.Find(active_version);
        if (!writer_opt)
        {
            continue;
        }

        RGPassNode& pass_to_activate = builder.pass_nodes[*writer_opt];
        if (!pass_to_activate.culled)
        {
            continue;
        }

        pass_to_activate.culled = false;

        // 이 패스가 읽는 리소스들을 활성 큐에 추가
        for (const RGResourceRef& read_ref : pass_to_activate.read_refs)
        {
            active_resource_queue.Push({ read_ref.resource_index, read_ref.version });
        }
    }

    // 4. 위상정렬 (Kahn's algorithm, O(N+E))
    const usize pass_count = builder.pass_nodes.Len();

    // 4-1. 각 패스의 read_refs를 순회하여 버전별 의존성에서 인접 리스트 + in_degree 구축
    Array<Array<uint32>> adjacency;
    adjacency.Resize(pass_count);

    Array<uint32> in_degrees;
    in_degrees.Resize(pass_count);
    std::ranges::fill(in_degrees, static_cast<uint32>(0));

    for (const auto [pass_idx, pass_node] : builder.pass_nodes | std::views::enumerate)
    {
        if (pass_node.culled)
        {
            continue;
        }

        const uint32 reader_idx = static_cast<uint32>(pass_idx);
        for (const RGResourceRef& read_ref : pass_node.read_refs)
        {
            const RGResourceNode& res_node = builder.resource_nodes[read_ref.resource_index];
            if (const Optional writer_opt = res_node.version_to_writer.Find(read_ref.version))
            {
                const uint32 writer_idx = *writer_opt;
                adjacency[writer_idx].Push(reader_idx);
                ++in_degrees[reader_idx];
            }
        }
    }

    // 4-2. BFS 위상정렬
    Queue<uint32> ready_queue;
    for (usize i = 0; i < pass_count; ++i)
    {
        if (!builder.pass_nodes[i].culled && in_degrees[i] == 0)
        {
            ready_queue.Push(static_cast<uint32>(i));
        }
    }

    while (Optional idx_opt = ready_queue.Pop())
    {
        const uint32 pass_idx = *idx_opt;
        compiled_passes.Push(&builder.pass_nodes[pass_idx]);

        for (const uint32 dependent_idx : adjacency[pass_idx])
        {
            if (--in_degrees[dependent_idx] == 0)
            {
                ready_queue.Push(dependent_idx);
            }
        }
    }

    // 4-3. 순환 의존성 확인
    const usize active_pass_count = std::ranges::count_if(
        builder.pass_nodes,
        [](const RGPassNode& p) { return !p.culled; }
    );

    if (compiled_passes.Len() != active_pass_count)
    {
        ConsoleLog(ELogLevel::Fatal, "A cycle was detected in the render graph!");
        for (usize i = 0; i < pass_count; ++i)
        {
            if (!builder.pass_nodes[i].culled && in_degrees[i] > 0)
            {
                ConsoleLog(ELogLevel::Fatal, "  - {}", builder.pass_nodes[i].name.ToString());
            }
        }
        SE_ASSERT(false, "A cycle was detected in the render graph!");
        return;
    }

    // 5. 리소스 생명주기 분석
    for (const auto [pass_idx, pass_node] : compiled_passes | std::views::enumerate)
    {
        const uint32 idx = static_cast<uint32>(pass_idx);

        for (const RGResourceRef& read_ref : pass_node->read_refs)
        {
            RGResourceNode& node = builder.resource_nodes[read_ref.resource_index];
            node.first_user_pass_index = std::min(node.first_user_pass_index, idx);
            node.last_user_pass_index  = std::max(node.last_user_pass_index, idx);
        }
        for (const auto& [res_idx, out_ver] : pass_node->write_map)
        {
            RGResourceNode& node = builder.resource_nodes[res_idx];
            node.first_user_pass_index = std::min(node.first_user_pass_index, idx);
            node.last_user_pass_index  = std::max(node.last_user_pass_index, idx);
        }
    }

    // 6. 리소스 할당/해제 스케줄 구축
    const usize compiled_pass_count = compiled_passes.Len();
    resources_to_realize.Resize(compiled_pass_count);
    resources_to_unrealize.Resize(compiled_pass_count);

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
