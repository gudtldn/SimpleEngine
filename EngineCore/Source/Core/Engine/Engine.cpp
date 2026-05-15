#include "SimpleEngine/Core/Engine/Engine.h"

#include "SimpleEngine/Core/Concurrency/AsyncFileIO.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/HAL/Platform.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"

#include "SDL3/SDL_gpu.h"
#include "tracy/Tracy.hpp"

#include <ranges>


namespace se
{
namespace
{
/**
 * TypeId 노드 집합에 대해 Kahn's 위상정렬을 수행합니다.
 *
 * @param nodes 정렬 대상 노드 목록
 * @param get_dependencies 각 노드의 의존성(선행 노드) 목록을 반환하는 콜백
 * @return 정렬된 TypeId 배열. 순환 의존성이 있으면 입력보다 짧은 배열이 반환됩니다.
 */
template <std::invocable<const TypeId&> DepsFn>
Array<TypeId> TopologicalSort(const Array<TypeId>& nodes, DepsFn&& get_dependencies)
{
    const HashSet<TypeId> node_set = HashSet<TypeId>::FromRange(nodes);

    HashMap<TypeId, Array<TypeId>> adj_list;
    HashMap<TypeId, int> in_degree;

    for (const TypeId& id : nodes)
    {
        in_degree[id] = 0;
        adj_list[id] = {};
    }

    for (const TypeId& id : nodes)
    {
        for (const TypeId& dep_id : get_dependencies(id))
        {
            // 노드 집합에 없는 의존성은 무시
            if (!node_set.Contains(dep_id))
            {
                continue;
            }

            adj_list[dep_id].Push(id);
            in_degree[id]++;
        }
    }

    Queue<TypeId> queue;
    for (const auto& [id, degree] : in_degree)
    {
        if (degree == 0)
        {
            queue.Push(id);
        }
    }

    Array<TypeId> sorted;
    while (Optional<TypeId> current = queue.Pop())
    {
        sorted.Push(*current);
        for (const TypeId& neighbor : adj_list[*current])
        {
            if (--in_degree[neighbor] == 0)
            {
                queue.Push(neighbor);
            }
        }
    }

    return sorted;
}
} // namespace


Engine* Engine::Instance = nullptr;

Engine::Engine()
{
    SE_ASSERT(!Instance, "Engine instance already exists.");
    Instance = this;

    // Interface Cache 구축
    TypeRegistry::Get().Resolve();

    VFS& vfs = VFS::Get();

    // 센티넬 파일(*.seproject) 탐색으로 프로젝트 루트 결정
    const Path root_path = Platform::FindProjectRoot();

    // bootstrap: Mount "Config://"
    vfs.Mount("Config", root_path / "Config");

    // EngineConfig.toml에서 VFS 마운트 포인트 로드
    if (auto result = ConfigFile::Load("Config://EngineConfig.toml"))
    {
        result->VisitSectionEntries("vfs", [&](StringView scheme, StringView relative_path)
        {
            vfs.Mount(scheme, root_path / relative_path);
        });
    }
    else
    {
        ConsoleLog(
            ELogLevel::Warning,
            "Failed to find 'EngineConfig.toml'. Using default VFS mounts and generating a default configuration file: {}", result.Error()
        );

        // 기본 VFS 마운트
        vfs.Mount("CoreAssets", root_path / "EngineCore/Assets");
        vfs.Mount("CoreShader", root_path / "EngineCore/Shaders");
        vfs.Mount("EditorAssets", root_path / "Editor/Assets");
        vfs.Mount("EditorShader", root_path / "Editor/Shaders");
        vfs.Mount("Cache", root_path / "Build/Cache");
        vfs.Mount("Logs", root_path / "Logs");

        // 기본 EngineConfig.toml 자동 생성
        GenerateDefaultEngineConfig();
    }

    // 쓰기 대상 스킴의 물리 디렉토리 보장
    vfs.EnsureDirectories({ "Cache", "Logs" });
}

Engine::~Engine()
{
    SE_ASSERT(Instance == this, "Engine instance is not initialized.");
    Instance = nullptr;
}

Engine& Engine::Get()
{
    SE_ASSERT(Instance, "Engine instance is not initialized.");
    return *Instance;
}

f64 Engine::GetDeltaTime()
{
    return Get().delta_time;
}

f64 Engine::GetElapsedTime()
{
    return Get().elapsed_time;
}

u64 Engine::GetFrameCount()
{
    return Get().frame_count;
}

void Engine::GenerateDefaultEngineConfig()
{
    // Config 디렉토리가 없을 수 있으므로 생성
    const Path config_dir = Platform::FindProjectRoot() / "Config";
    if (!config_dir.Exists())
    {
        FileSystem::CreateDirectories(config_dir);
    }

    ConfigFile config;
    config.SetValue("vfs.CoreAssets", String("EngineCore/Assets"));
    config.SetValue("vfs.CoreShader", String("EngineCore/Shaders"));
    config.SetValue("vfs.EditorAssets", String("Editor/Assets"));
    config.SetValue("vfs.EditorShader", String("Editor/Shaders"));
    config.SetValue("vfs.Cache", String("Cache"));
    config.SetValue("vfs.Logs", String("Logs"));

    if (config.Save("Config://EngineConfig.toml"))
    {
        ConsoleLog(ELogLevel::Info, "Successfully created default 'EngineConfig.toml'.");
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "Failed to create default 'EngineConfig.toml'.");
    }
}

void Engine::LoadRegisteredSubsystems()
{
    auto& registry = detail::SubsystemRegistry::GetInstance();
    for (const auto& [type_id, metadata] : registry.GetMetadataMap())
    {
        if (subsystems.Contains(type_id))
        {
            continue;
        }

        if (std::unique_ptr<SubsystemBase> subsystem = metadata.factory())
        {
            subsystems.Emplace(type_id, std::move(subsystem));
            ConsoleLog(ELogLevel::Debug, "Instantiated Subsystem: {}", type_id.GetName());
        }
    }
}

SubsystemBase* Engine::GetSubsystem(const TypeId& type_id) const
{
    if (const auto subsystem = subsystems.Find(type_id))
    {
        return subsystem->get();
    }
    return nullptr;
}

bool Engine::Initialize()
{
    // SDL 코어 시스템 초기화
    if (!SDL_Init(0))
    {
        ConsoleLog(ELogLevel::Fatal, "SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    job_system = std::make_unique<JobSystem>();
    async_io_service = std::make_unique<AsyncFileIO>();

    // 의존성에 따라서 정렬
    if (!SortSubsystems())
    {
        return false;
    }

    // SusSystems 초기화
    if (!InitializeAllSubsystems())
    {
        ConsoleLog(ELogLevel::Error, "Subsystems failed to initialize!");
        return false;
    }

    return true;
}

void Engine::Release()
{
    ReleaseAllSubsystems();
    sorted_subsystems.Clear();
    subsystems.Clear();

    async_io_service.reset();
    job_system.reset();
    JobAllocator::Shutdown();

    SDL_Quit();
}

void Engine::UpdateFrame(f64 in_delta_time)
{
#define SE_PROFILE_SCOPE(scope_fmt, ...) \
    ZoneScoped; \
    SE_DEBUG_EXPRESSION({ \
        const se::String zone_name = se::String::Format("Engine::UpdateFrame - " scope_fmt __VA_OPT__(,) __VA_ARGS__); \
        ZoneName(zone_name.CStr(), zone_name.ByteLen()); \
    })

    delta_time = in_delta_time;
    elapsed_time += in_delta_time;
    ++frame_count;

    for (const auto& [subsystem, name] : updatable_systems)
    {
        SE_PROFILE_SCOPE("PreUpdate | SubSystem: {}", name);
        subsystem->PreUpdate();
    }
    for (const auto& [subsystem, name] : updatable_systems)
    {
        SE_PROFILE_SCOPE("Update | SubSystem: {}", name);
        subsystem->Update(delta_time);
    }
    for (const auto& [subsystem, name] : updatable_systems)
    {
        SE_PROFILE_SCOPE("PostUpdate | SubSystem: {}", name);
        subsystem->PostUpdate();
    }

    {
        SE_PROFILE_SCOPE("MainThreadTasks");

        // 비동기 태스크를 마저 실행
        job_system->ExecuteMainThreadJobs();
    }

#undef SE_PROFILE_SCOPE
}

bool Engine::InitializeAllSubsystems()
{
    ConsoleLog(ELogLevel::Info, "Initializing Subsystems...");
    for (auto [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        if (!sub_system->Initialize())
        {
            ConsoleLog(ELogLevel::Error, "Subsystem {} failed to initialize!", sub_system->GetTypeId().GetName());

            const auto subrange = std::ranges::subrange(sorted_subsystems.begin(), sorted_subsystems.begin() + n);
            for (SubsystemBase* rev_subsystem : subrange | std::views::reverse)
            {
                rev_subsystem->Release();
            }
            return false;
        }
    }
    ConsoleLog(ELogLevel::Info, "All Subsystems initialized successfully");
    return true;
}

void Engine::ReleaseAllSubsystems()
{
    ConsoleLog(ELogLevel::Info, "Releasing Subsystems...");

    // RenderSubsystem이 있다면, 해제하기전에 GPU 대기
    if (const RenderSubsystem* render_subsystem = GetSubsystem<const RenderSubsystem>())
    {
        SDL_WaitForGPUIdle(render_subsystem->GetRenderDevice().GetRawDevice());
    }

    for (SubsystemBase* sub_system : sorted_subsystems | std::views::reverse)
    {
        sub_system->Release();
    }
    ConsoleLog(ELogLevel::Info, "All Subsystems released successfully");
}

bool Engine::SortSubsystems()
{
    ConsoleLog(ELogLevel::Info, "Sorting Subsystems based on dependencies...");

    detail::SubsystemRegistry& registry = detail::SubsystemRegistry::GetInstance();

    // 모든 서브시스템의 TypeId 수집
    Array<TypeId> all_ids;
    for (const TypeId& type_id : subsystems | std::views::keys)
    {
        all_ids.Push(type_id);
    }

    // 초기화 순서 위상정렬
    Array<TypeId> sorted_ids = TopologicalSort(all_ids, [&](const TypeId& id) -> const Array<TypeId>&
    {
        return registry.GetMetadata(id).dependencies;
    });

    if (sorted_ids.Len() != all_ids.Len())
    {
        ConsoleLog(ELogLevel::Fatal, "Circular dependency detected among Subsystems! Sorting failed.");
        HashSet<TypeId> sorted_set(sorted_ids.begin(), sorted_ids.end());
        for (const TypeId& id : all_ids)
        {
            if (!sorted_set.Contains(id))
            {
                ConsoleLog(ELogLevel::Fatal, "- {}", id.GetName());
            }
        }
        return false;
    }

    sorted_subsystems.Clear();
    for (const TypeId& id : sorted_ids)
    {
        sorted_subsystems.Push(subsystems[id].get());
    }

    ConsoleLog(ELogLevel::Info, "Subsystems sorted successfully.");
    for (const auto [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        ConsoleLog(ELogLevel::Debug, "  - Init Order {}: {}", n, sub_system->GetTypeId().GetName());
    }

    // IUpdatable을 update_dependencies에 따라 별도 위상정렬
    // UpdateDependsOn이 없는 서브시스템은 다른 IUpdatable과의 순서가 보장되지 않음
    HashMap<TypeId, IUpdatable*> updatable_map;
    Array<TypeId> updatable_ids;
    for (const auto& [type_id, subsystem_ptr] : subsystems)
    {
        if (IUpdatable* updatable = Cast<IUpdatable>(subsystem_ptr.get()))
        {
            updatable_map.Emplace(type_id, updatable);
            updatable_ids.Push(type_id);
        }
    }

    Array<TypeId> sorted_update_ids = TopologicalSort(updatable_ids, [&](const TypeId& id) -> const Array<TypeId>&
    {
        return registry.GetMetadata(id).update_dependencies;
    });

    if (sorted_update_ids.Len() != updatable_ids.Len())
    {
        ConsoleLog(ELogLevel::Fatal, "Circular update dependency detected among IUpdatable subsystems!");
        HashSet<TypeId> sorted_set(sorted_update_ids.begin(), sorted_update_ids.end());
        for (const TypeId& id : updatable_ids)
        {
            if (!sorted_set.Contains(id))
            {
                ConsoleLog(ELogLevel::Fatal, "- {}", id.GetName());
            }
        }
        return false;
    }

    updatable_systems.Clear();
    for (const TypeId& id : sorted_update_ids)
    {
        updatable_systems.Push({
            .updatable = updatable_map[id],
            .name = id.GetName(),
        });
    }

    ConsoleLog(ELogLevel::Debug, "IUpdatable subsystems update order:");
    for (const auto [n, entry] : updatable_systems | std::views::enumerate)
    {
        ConsoleLog(ELogLevel::Debug, "  - Update Order {}: {}", n, entry.name);
    }

    return true;
}
} // namespace se
