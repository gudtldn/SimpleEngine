#include "Core/Engine/Engine.h"

#include <ranges>

#include "Core/Concurrency/TaskScheduler.h"
#include "Core/Concurrency/ThreadPool.h"
#include "Core/Container/Array.h"
#include "Core/Container/HashMap.h"
#include "Core/Container/Queue.h"
#include "Core/Logging/Logging.h"
#include "Core/Subsystem/ISubsystem.h"
#include "Core/Subsystem/IUpdatable.h"
#include "Core/Subsystem/SubsystemRegistration.h"
#include "Gfx/RenderSubsystem.h"
#include "Utility/VFS.h"

#include "SDL3/SDL_gpu.h"
#include "tracy/Tracy.hpp"


namespace se
{
Engine* Engine::Instance = nullptr;

Engine::Engine()
{
    SE_ASSERT(!Instance, "Engine instance already exists.");
    Instance = this;

    VFS& vfs = VFS::Get();

    // TODO: Shipping일 때 GetExecutableDirectory로 수정해야함!!!
    const Path solution_path = PROJECT_ROOT_DIR;

    // Core
    vfs.Mount("Config", solution_path / "Config");
    vfs.Mount("CoreAssets", solution_path / "EngineCore/Assets");
    vfs.Mount("CoreShader", solution_path / "EngineCore/Shaders");

    // Editor
    vfs.Mount("EditorAssets", solution_path / "Editor/Assets");
    vfs.Mount("EditorShader", solution_path / "Editor/Shaders");
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

void Engine::LoadRegisteredSubsystems()
{
    auto& registry = detail::SubsystemRegistry::GetInstance();
    for (const auto& [type_id, metadata] : registry.GetMetadataMap())
    {
        if (subsystems.Contains(type_id))
        {
            continue;
        }

        if (std::unique_ptr<ISubsystem> subsystem = metadata.factory())
        {
            if (IUpdatable* updatable = dynamic_cast<IUpdatable*>(subsystem.get()))
            {
                updatable_systems.Push(updatable);
            }

            subsystems.Emplace(type_id, std::move(subsystem));
            ConsoleLog(ELogLevel::Debug, "Instantiated Subsystem: {}", type_id.GetName());
        }
    }
}

bool Engine::Initialize()
{
    task_scheduler = std::make_unique<TaskScheduler>(std::this_thread::get_id());

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

    task_scheduler.reset();
}

bool Engine::InitializeAllSubsystems()
{
    ConsoleLog(ELogLevel::Info, "Initializing Subsystems...");
    for (auto [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        if (!sub_system->Initialize())
        {
            ConsoleLog(ELogLevel::Error, "Subsystem {} failed to initialize!", typeid(*sub_system).name());

            const auto subrange = std::ranges::subrange(sorted_subsystems.begin(), sorted_subsystems.begin() + n);
            for (ISubsystem* rev_subsystem : subrange | std::views::reverse)
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
        SDL_WaitForGPUIdle(render_subsystem->GetGpuDevice());
    }

    for (ISubsystem* sub_system : sorted_subsystems | std::views::reverse)
    {
        sub_system->Release();
    }
    ConsoleLog(ELogLevel::Info, "All Subsystems released successfully");
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Engine::UpdateFrame(float delta_time)
{
#define SE_PROFILE_SCOPE(scope_fmt, ...) \
    ZoneScoped; \
    SE_DEBUG_EXPRESION({ \
        const se::String zone_name = se::String::Format("Engine::UpdateFrame - " scope_fmt __VA_OPT__(,) __VA_ARGS__); \
        ZoneName(zone_name.CStr(), zone_name.ByteLen()); \
    })

    for (IUpdatable* sub_system : updatable_systems)
    {
        SE_PROFILE_SCOPE("PreUpdate | SubSystem: {}", typeid(*sub_system).name());
        sub_system->PreUpdate();
    }
    for (IUpdatable* sub_system : updatable_systems)
    {
        SE_PROFILE_SCOPE("Update | SubSystem: {}", typeid(*sub_system).name());
        sub_system->Update(delta_time);
    }
    for (IUpdatable* sub_system : updatable_systems)
    {
        SE_PROFILE_SCOPE("PostUpdate | SubSystem: {}", typeid(*sub_system).name());
        sub_system->PostUpdate();
    }

    {
        SE_PROFILE_SCOPE("MainThreadTasks");

        // 비동기 태스크를 마저 실행
        task_scheduler->ProcessMainThreadTasks();
    }

#undef SE_PROFILE_SCOPE
}

bool Engine::SortSubsystems()
{
    ConsoleLog(ELogLevel::Info, "Sorting Subsystems based on dependencies...");

    HashMap<refl::TypeId, Array<refl::TypeId>> adj_list;
    HashMap<refl::TypeId, int> in_degree;
    Queue<refl::TypeId> queue;

    // 의존성 그래프와 진입 차수(in-degree)를 계산
    for (const refl::TypeId& type_id : subsystems | std::views::keys)
    {
        in_degree[type_id] = 0; // 모든 노드의 진입 차수 0으로 초기화
        adj_list[type_id] = {}; // 인접 리스트 초기화
    }

    auto& registry = detail::SubsystemRegistry::GetInstance();
    for (const auto& type_id : subsystems | std::views::keys)
    {
        for (const refl::TypeId& dependency_id : registry.GetMetadata(type_id).dependencies)
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
        const refl::TypeId current_id = *current_id_opt;
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

        Array<refl::TypeId> circular_subsystems;
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

    // Update 순서는 한번 보고 나중에 필요시 변경

    ConsoleLog(ELogLevel::Info, "Subsystems sorted successfully.");
    for (const auto [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        ConsoleLog(ELogLevel::Debug, "  - Order {}: {}", n, typeid(*sub_system).name());
    }

    return true;
}
}  // namespace se
