#include "SimpleEngine/Core/Engine/Engine.h"

#include "SimpleEngine/Core/Concurrency/AsyncFileIO.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
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
        result.Value().VisitSectionEntries("vfs", [&](StringView scheme, StringView relative_path)
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

// ReSharper disable once CppMemberFunctionMayBeConst
void Engine::UpdateFrame(float delta_time)
{
#define SE_PROFILE_SCOPE(scope_fmt, ...) \
    ZoneScoped; \
    SE_DEBUG_EXPRESSION({ \
        const se::String zone_name = se::String::Format("Engine::UpdateFrame - " scope_fmt __VA_OPT__(,) __VA_ARGS__); \
        ZoneName(zone_name.CStr(), zone_name.ByteLen()); \
    })

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

    HashMap<TypeId, Array<TypeId>> adj_list;
    HashMap<TypeId, int> in_degree;
    Queue<TypeId> queue;

    // 의존성 그래프와 진입 차수(in-degree)를 계산
    for (const TypeId& type_id : subsystems | std::views::keys)
    {
        in_degree[type_id] = 0; // 모든 노드의 진입 차수 0으로 초기화
        adj_list[type_id] = {}; // 인접 리스트 초기화
    }

    auto& registry = detail::SubsystemRegistry::GetInstance();
    for (const auto& type_id : subsystems | std::views::keys)
    {
        for (const TypeId& dependency_id : registry.GetMetadata(type_id).dependencies)
        {
            // A가 B에 의존한다면 (A -> B), B에서 A로 가는 간선을 추가
            // B가 먼저 초기화되어야 하기 때문
            adj_list[dependency_id].Push(type_id);
            in_degree[type_id]++;
        }
    }

    // 진입 차수가 0인 노드들을 큐에 추가
    // 이 노드들은 다른 어떤 노드에도 의존하지 않으므로 초기화 순서의 시작점
    for (const auto& [type_id, degree] : in_degree)
    {
        if (degree == 0)
        {
            queue.Push(type_id);
        }
    }

    // 위상 정렬을 수행
    sorted_subsystems.Clear();
    while (Optional current_id_opt = queue.Pop())
    {
        const TypeId current_id = *current_id_opt;
        sorted_subsystems.Push(subsystems[current_id].get());

        for (const auto& neighbor_id : adj_list[current_id])
        {
            --in_degree[neighbor_id];
            if (in_degree[neighbor_id] == 0)
            {
                queue.Push(neighbor_id);
            }
        }
    }

    // 순환 의존성 확인
    if (sorted_subsystems.Len() != subsystems.Len())
    {
        ConsoleLog(ELogLevel::Fatal, "Circular dependency detected among Subsystems! Sorting failed.");

        Array<TypeId> circular_subsystems;
        for (const auto& [type_id, degree] : in_degree)
        {
            if (degree > 0)
            {
                circular_subsystems.Push(type_id);
            }
        }

        ConsoleLog(ELogLevel::Fatal, "Circular dependency detected in subsystems: ");
        for (const auto& id : circular_subsystems)
        {
            ConsoleLog(ELogLevel::Fatal, "- {}", id.GetName());
        }

        return false;
    }

    // 일단 sorted_subsystems 순서에 따라 updatable_systems 등록
    updatable_systems.Clear();
    for (SubsystemBase* sub_system : sorted_subsystems)
    {
        if (IUpdatable* updatable = Cast<IUpdatable>(sub_system))
        {
            updatable_systems.Push({
                .updatable = updatable,
                .name = sub_system->GetTypeId().GetName(),
            });
        }
    }

    ConsoleLog(ELogLevel::Info, "Subsystems sorted successfully.");
    for (const auto [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        ConsoleLog(ELogLevel::Debug, "  - Order {}: {}", n, sub_system->GetTypeId().GetName());
    }

    return true;
}
}  // namespace se
