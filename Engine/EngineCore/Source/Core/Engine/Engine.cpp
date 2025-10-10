module;
#include <typeinfo>
#include "tracy/Tracy.hpp"
module SE.Core;
import :Engine;
import :Paths;

import SE.Utility;
import SE.Subsystems.RenderSubsystem;

import "SDL3/SDL_gpu.h";


namespace se::core::engine
{
Engine::Engine()
{
    paths::PathResolver& path_resolver = paths::PathResolver::Get();

    // TODO: Shipping일 때 경로 수정해야함!!!
    const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();

    // Core
    path_resolver.Mount(u8"Config", solution_path / u8"Config");
    path_resolver.Mount(u8"CoreAssets", solution_path / u8"Engine/EngineCore/Assets");
    path_resolver.Mount(u8"CoreShader", solution_path / u8"Engine/EngineCore/Shaders");

    // Editor
    path_resolver.Mount(u8"EditorAssets", solution_path / u8"Engine/EditorEngine/Assets");
    path_resolver.Mount(u8"EditorShader", solution_path / u8"Engine/EditorEngine/Shaders");
}

bool Engine::Initialize()
{
    thread_pool.reset(new concurrency::ThreadPool{
        static_cast<uint32>(std::thread::hardware_concurrency() * 0.7)
    });
    task_scheduler.reset(new concurrency::TaskScheduler{
        std::this_thread::get_id()
    });

    // 의존성에 따라서 정렬
    if (!SortSubsystems())
    {
        return false;
    }

    // SusSystems 초기화
    if (!InitializeAllSubsystems())
    {
        ConsoleLog(ELogLevel::Error, u8"Subsystems failed to initialize!");
        return false;
    }

    return true;
}

void Engine::Release()
{
    ReleaseAllSubsystems();
    sorted_subsystems.clear();
    subsystems.clear();

    thread_pool.reset();
    task_scheduler.reset();
}

bool Engine::InitializeAllSubsystems()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing Subsystems...");
    for (auto [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        if (!sub_system->Initialize())
        {
            const u8string sub_system_name = utility::string::ToU8String(typeid(*sub_system).name());
            ConsoleLog(ELogLevel::Error, u8"Subsystem {} failed to initialize!", sub_system_name);

            const auto subrange = std::ranges::subrange(sorted_subsystems.begin(), sorted_subsystems.begin() + n);
            for (ISubsystemBase* rev_subsystem : subrange | std::views::reverse)
            {
                rev_subsystem->Release();
            }
            return false;
        }
    }
    ConsoleLog(ELogLevel::Info, u8"All Subsystems initialized successfully");
    return true;
}

void Engine::ReleaseAllSubsystems()
{
    ConsoleLog(ELogLevel::Info, u8"Releasing Subsystems...");

    // RenderSubsystem이 있다면, 해제하기전에 GPU 대기
    if (const RenderSubsystem* render_subsystem = GetSubsystem<RenderSubsystem>())
    {
        SDL_WaitForGPUIdle(render_subsystem->GetGpuDevice());
    }

    for (ISubsystemBase* sub_system : sorted_subsystems | std::views::reverse)
    {
        sub_system->Release();
    }
    ConsoleLog(ELogLevel::Info, u8"All Subsystems released successfully");
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Engine::UpdateFrame(float delta_time)
{
#define SE_PROFILE_SCOPE(scope_fmt, ...) \
    ZoneScoped; \
    { \
        [[maybe_unused]] const std::string zone_name = std::format("Engine::UpdateFrame - " scope_fmt __VA_OPT__(,) __VA_ARGS__); \
        ZoneName(zone_name.c_str(), zone_name.size()); \
    }

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
    ConsoleLog(ELogLevel::Info, u8"Sorting Subsystems based on dependencies...");

    unordered_map<std::type_index, vector<std::type_index>> adj_list;
    unordered_map<std::type_index, int> in_degree;
    queue<std::type_index> queue;

    // 의존성 그래프와 진입 차수(in-degree)를 계산
    for (const std::type_index& type_id : subsystems | std::views::keys)
    {
        in_degree[type_id] = 0; // 모든 노드의 진입 차수 0으로 초기화
        adj_list[type_id] = {}; // 인접 리스트 초기화
    }

    for (const auto& [type_id, sub_system] : subsystems)
    {
        for (const auto& dependency_id : sub_system->GetDependencies())
        {
            // A가 B에 의존한다면 (A -> B), B에서 A로 가는 간선을 추가
            // B가 먼저 초기화되어야 하기 때문
            adj_list[dependency_id].push_back(type_id);
            in_degree[type_id]++;
        }
    }

    // 진입 차수가 0인 노드들을 큐에 추가
    // 이 노드들은 다른 어떤 노드에도 의존하지 않으므로 초기화 순서의 시작점
    for (const auto& [type_id, degree] : in_degree)
    {
        if (degree == 0)
        {
            queue.push(type_id);
        }
    }

    // 위상 정렬을 수행
    sorted_subsystems.clear();
    while (!queue.empty())
    {
        const std::type_index current_id = queue.front();
        queue.pop();

        sorted_subsystems.push_back(subsystems[current_id].get());

        for (const auto& neighbor_id : adj_list[current_id])
        {
            --in_degree[neighbor_id];
            if (in_degree[neighbor_id] == 0)
            {
                queue.push(neighbor_id);
            }
        }
    }

    // 순환 의존성 확인
    if (sorted_subsystems.size() != subsystems.size())
    {
        ConsoleLog(ELogLevel::Fatal, u8"Circular dependency detected among Subsystems! Sorting failed.");

        vector<std::type_index> circular_subsystems;
        for (const auto& [type_id, degree] : in_degree)
        {
            if (degree > 0)
            {
                circular_subsystems.push_back(type_id);
            }
        }

        ConsoleLog(ELogLevel::Fatal, u8"Circular dependency detected in subsystems: ");
        for (const auto& id : circular_subsystems)
        {
            ConsoleLog(ELogLevel::Fatal, u8"- {}", typeid(*subsystems[id]).name());
        }

        return false;
    }

    // Update 순서는 한번 보고 나중에 필요시 변경

    ConsoleLog(ELogLevel::Info, u8"Subsystems sorted successfully.");
    for (const auto& [n, sub_system] : sorted_subsystems | std::views::enumerate)
    {
        ConsoleLog(ELogLevel::Debug, u8"  - Order {}: {}", n, utility::string::ToU8String(typeid(*sub_system).name()));
    }

    return true;
}
}
