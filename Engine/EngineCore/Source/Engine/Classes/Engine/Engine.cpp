module;
#include <typeinfo>
module SimpleEngine.Engine;

import SimpleEngine.Utils;
import std;


bool Engine::Initialize()
{
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
    sub_systems.clear();
    sorted_sub_systems.clear();
}

bool Engine::InitializeAllSubsystems()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing Subsystems...");
    for (ISubsystem* sub_system : sorted_sub_systems)
    {
        if (!sub_system->Initialize())
        {
            const std::u8string sub_system_name = se::string_utils::ToU8String(typeid(*sub_system).name());
            ConsoleLog(ELogLevel::Error, u8"Subsystem {} failed to initialize!", sub_system_name);

            const auto current_it = std::ranges::find(sorted_sub_systems, sub_system);
            const auto subrange = std::ranges::subrange(sorted_sub_systems.begin(), current_it);
            for (ISubsystem* rev_subsystem : subrange | std::views::reverse)
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
    for (ISubsystem* sub_system : sorted_sub_systems | std::views::reverse)
    {
        sub_system->Release();
    }
    ConsoleLog(ELogLevel::Info, u8"All Subsystems released successfully");
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Engine::UpdateFrame(float delta_time)
{
    for (IUpdatable* sub_system : updatable_systems)
    {
        sub_system->Update(delta_time);
    }
}

bool Engine::SortSubsystems()
{
    ConsoleLog(ELogLevel::Info, u8"Sorting Subsystems based on dependencies...");

    std::unordered_map<std::type_index, std::vector<std::type_index>> adj_list;
    std::unordered_map<std::type_index, int> in_degree;
    std::queue<std::type_index> queue;

    // 의존성 그래프와 진입 차수(in-degree)를 계산
    for (const std::type_index& type_id : sub_systems | std::views::keys)
    {
        in_degree[type_id] = 0; // 모든 노드의 진입 차수 0으로 초기화
        adj_list[type_id] = {}; // 인접 리스트 초기화
    }

    for (const auto& [type_id, sub_system] : sub_systems)
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
    sorted_sub_systems.clear();
    while (!queue.empty())
    {
        const std::type_index current_id = queue.front();
        queue.pop();

        sorted_sub_systems.push_back(sub_systems[current_id].get());

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
    if (sorted_sub_systems.size() != sub_systems.size())
    {
        ConsoleLog(ELogLevel::Fatal, u8"Circular dependency detected among Subsystems! Sorting failed.");

        std::vector<std::type_index> circular_subsystems;
        for (const auto& [type_id, degree] : in_degree)
        {
            if (degree > 0)
            {
                circular_subsystems.push_back(type_id);
            }
        }

        std::u8string error_msg = u8"Circular dependency detected in subsystems: ";
        for (const auto& id : circular_subsystems)
        {
            error_msg += u8"\n  - " + se::string_utils::ToU8String(typeid(*sub_systems[id]).name());
        }

        ConsoleLog(ELogLevel::Fatal, error_msg);
        return false;
    }

    // Update 순서는 한번 보고 나중에 필요시 변경

    ConsoleLog(ELogLevel::Info, u8"Subsystems sorted successfully.");
    for (const auto& [n, sub_system] : sorted_sub_systems | std::views::enumerate)
    {
        ConsoleLog(ELogLevel::Info, u8"  - Order {}: {}", n, se::string_utils::ToU8String(typeid(*sub_system).name()));
    }

    return true;
}
