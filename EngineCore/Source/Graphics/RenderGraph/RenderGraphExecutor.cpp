// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphExecutor.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "SimpleEngine/Graphics/RenderGraph/RGContexts.h"
#include "SimpleEngine/Utility/Debug.h"
#include "SimpleEngine/Utility/GpuDebug.h"

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

RenderGraphExecutor::~RenderGraphExecutor() = default;

void RenderGraphExecutor::Execute(RenderGraphBuilder& builder, SDL_GPUCommandBuffer* cmd, PSOManager& pso_manager)
{
    ZoneScoped;

    Compile(builder);

    for (const auto [pass_idx, pass_node] : compiled_passes | std::views::enumerate)
    {
        ZoneScoped;
        SE_DEBUG_EXPRESSION({
            const String zone_name = String::Format("RenderGraphExecutor::Execute - {}::Execute", pass_node->name);
            ZoneName(zone_name.CStr(), zone_name.ByteLen());
        })

        // GPU Debug Group 설정
        SE_GPU_DEBUG_GROUP(cmd, pass_node->name.CStr());

        // 이 패스에서 생명주기가 시작되는 리소스를 할당
        for (const usize resource_idx : resources_to_realize[pass_idx])
        {
            builder.resource_nodes[resource_idx].resource->Realize(resource_pool);
        }

        RGExecutionContext context{ cmd, pso_manager, builder };
        pass_node->pass_object->Execute(context);

        // 이 패스에서 생명주기가 끝나는 리소스를 해제
        for (const usize resource_idx : resources_to_unrealize[pass_idx])
        {
            builder.resource_nodes[resource_idx].resource->Unrealize(resource_pool);
        }
    }

    // Execute()중 Unrealize되지 못한 리소스 정리
    for (const RGResourceNode& node : builder.resource_nodes)
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

    // 1. Setup 및 자동 버저닝 (SSA: Static Single Assignment 구성)
    //    AddPass 순서대로 물리적 리소스에 논리적 버전(Version)을 자동 부여하여 의존성 방향 그래프(DAG)를 구성

    // 각 물리적 리소스(index)별로 현재까지 도달한 '최신 버전'을 추적하기 위한 배열 (초기값 0)
    Array<uint32> current_versions;
    current_versions.Resize(builder.resource_nodes.Len());
    std::ranges::fill(current_versions, static_cast<uint32>(0));

    for (const auto [pass_idx, pass_node] : builder.pass_nodes | std::views::enumerate)
    {
        ZoneScoped;
        SE_DEBUG_EXPRESSION({
            const String zone_name = String::Format("RenderGraphExecutor::Compile - {}::Setup", pass_node.name.ToString());
            ZoneName(zone_name.CStr(), zone_name.ByteLen());
        });

        // Setup()으로 read_indices와 write_indices를 수집
        RGSetupContext setup_ctx(pass_node);
        pass_node.pass_object->Setup(setup_ctx);

        // --- A. Read 처리 ---
        // 해당 리소스가 지금까지 다른 패스들에 의해 갱신된 '가장 최신 버전'을 의존성으로 기록
        for (const uint32 res_idx : pass_node.read_indices)
        {
#if SE_BUILD_DEBUG
            if (res_idx >= builder.resource_nodes.Len())
            {
                ConsoleLog(
                    ELogLevel::Error, "Invalid resource index {} in {}::Setup() (read_indices)",
                    res_idx, pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Invalid resource index.");
                continue;
            }
            if (!builder.resource_nodes[res_idx].resource)
            {
                ConsoleLog(
                    ELogLevel::Error, "Resource '{}' is not initialized. Check {}::Setup()",
                    builder.resource_nodes[res_idx].name.ToString(),
                    pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Resource is not initialized.");
                continue;
            }
#endif
            pass_node.read_refs.Push({ .resource_index = res_idx, .version = current_versions[res_idx] });
        }

        // --- B. Write 처리 ---
        // 리소스에 Write를 한다는 것은 물리적 메모리의 내용이 변경되어 '새로운 논리적 상태'가 됨을 의미
        for (const uint32 res_idx : pass_node.write_indices)
        {
#if SE_BUILD_DEBUG
            // 유효성 검사: 존재하지 않거나 초기화되지 않은 리소스 인덱스에 접근하는지 확인
            if (res_idx >= builder.resource_nodes.Len())
            {
                ConsoleLog(
                    ELogLevel::Error, "Invalid resource index {} in {}::Setup()",
                    res_idx, pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Invalid resource index.");
                continue;
            }
            if (!builder.resource_nodes[res_idx].resource)
            {
                ConsoleLog(
                    ELogLevel::Error, "Resource {} is not initialized. Check {}::Setup()",
                    builder.resource_nodes[res_idx].name.ToString(),
                    pass_node.pass_object->GetTypeId().GetName()
                );
                SE_ASSERT(false, "Resource is not initialized.");
                continue;
            }
#endif

            const uint32 in_version = current_versions[res_idx];
            const uint32 out_version = in_version + 1;
            current_versions[res_idx] = out_version; // 버전 리스트 업데이트

            // 해당 버전을 '이 패스가 생성(Write)했다'고 기록
            pass_node.write_map[res_idx] = out_version;
            builder.resource_nodes[res_idx].version_to_writer[out_version] = static_cast<uint32>(pass_idx);

            // WAW(Write-After-Write) 의존성 보장 및 암묵적 Read
            // 명시적인 Read 선언이 없더라도, 동일 리소스에 대한 쓰기 작업(AddPass) 순서가 보장되도록
            // 이전 버전에 대한 암묵적 Read를 기록하여 의존성이 끊기는 것을 방지
            if (in_version > 0)
            {
                pass_node.read_refs.Push({ .resource_index = res_idx, .version = in_version });
            }
        }
    }

    // 2. Pass Culling: 외부 리소스에서 역방향으로 활성 패스를 조회
    //    외부 리소스의 최신 버전을 생성한 패스부터 역탐색합니다.
    struct ActiveResourceEntry { uint32 index; uint32 version; };
    Queue<ActiveResourceEntry> active_resource_queue;

    for (const auto [res_idx, res_node] : builder.resource_nodes | std::views::enumerate)
    {
        const RGResourceBase* resource = res_node.resource.get();
        if (!IsA<RGExternalTexture>(resource) && !IsA<RGExternalBuffer>(resource))
        {
            continue;
        }

        // 외부 리소스의 최신 버전(가장 높은 버전)을 활성화 트리거로 사용
        uint32 max_version = 0;
        bool has_writer = false;
        for (const uint32& ver : res_node.version_to_writer | std::views::keys)
        {
            if (!has_writer || ver > max_version)
            {
                max_version = ver;
                has_writer = true;
            }
        }

        if (has_writer)
        {
            active_resource_queue.Push({ .index = static_cast<uint32>(res_idx), .version = max_version });
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
            active_resource_queue.Push({ .index = read_ref.resource_index, .version = read_ref.version });
        }
    }

    // 2-2. 외부 리소스를 읽기만 하는 패스 활성화
    //      외부 리소스는 이미 Realize된 상태이므로, 해당 리소스를 읽는 패스는 항상 유효합니다.
    //      (예: Readback 패스, TAA history 읽기 패스 등)
    for (const auto [pass_idx, pass_node] : builder.pass_nodes | std::views::enumerate)
    {
        if (!pass_node.culled)
        {
            continue;
        }

        bool reads_external = false;
        for (const RGResourceRef& read_ref : pass_node.read_refs)
        {
            const RGResourceBase* resource = builder.resource_nodes[read_ref.resource_index].resource.get();
            if (IsA<RGExternalTexture>(resource) || IsA<RGExternalBuffer>(resource))
            {
                reads_external = true;
                break;
            }
        }

        if (reads_external)
        {
            pass_node.culled = false;
            for (const RGResourceRef& read_ref : pass_node.read_refs)
            {
                active_resource_queue.Push({ .index = read_ref.resource_index, .version = read_ref.version });
            }
        }
    }

    // 새로 활성화된 패스의 트랜지티브 의존성 역방향 BFS
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
        for (const RGResourceRef& read_ref : pass_to_activate.read_refs)
        {
            active_resource_queue.Push({ .index = read_ref.resource_index, .version = read_ref.version });
        }
    }

    // 3. 위상정렬 (Kahn's algorithm, O(N+E))
    const usize pass_count = builder.pass_nodes.Len();

    // 3-1. 각 패스의 read_refs를 순회하여 버전별 의존성에서 인접 리스트 + in_degree 구축
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

    // 3-2. BFS 위상정렬
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

    // 3-3. 순환 의존성 확인
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

    // 4. 리소스 생명주기 분석
    for (const auto [pass_idx, pass_node] : compiled_passes | std::views::enumerate)
    {
        const uint32 idx = static_cast<uint32>(pass_idx);

        for (const RGResourceRef& read_ref : pass_node->read_refs)
        {
            RGResourceNode& node = builder.resource_nodes[read_ref.resource_index];
            node.first_user_pass_index = std::min(node.first_user_pass_index, idx);
            node.last_user_pass_index  = std::max(node.last_user_pass_index, idx);
        }
        for (const uint32& res_idx : pass_node->write_map | std::views::keys)
        {
            RGResourceNode& node = builder.resource_nodes[res_idx];
            node.first_user_pass_index = std::min(node.first_user_pass_index, idx);
            node.last_user_pass_index  = std::max(node.last_user_pass_index, idx);
        }
    }

    // 5. 리소스 할당/해제 스케줄 구축
    const usize compiled_pass_count = compiled_passes.Len();
    resources_to_realize.Resize(compiled_pass_count);
    resources_to_unrealize.Resize(compiled_pass_count);

    for (const auto [res_idx, node] : builder.resource_nodes | std::views::enumerate)
    {
        const uint32 idx = static_cast<uint32>(res_idx);

        if (node.first_user_pass_index <= node.last_user_pass_index)
        {
            resources_to_realize[node.first_user_pass_index].Push(idx);
            resources_to_unrealize[node.last_user_pass_index].Push(idx);
        }
    }
}
} // namespace se::graphics
