#include "SimpleEditor/Asset/DependencyGraph.h"

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Debug.h"

#include "tracy/Tracy.hpp"

#include <tuple>


namespace se::editor
{
void DependencyGraph::SetDependencies(const AssetId& id, ArrayView<const AssetId> dependencies)
{
    ZoneScopedN("DependencyGraph::SetDependencies");
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    // 0. 입력 중복 제거 + 자기 참조 필터링
    Array<AssetId> unique_deps;
    {
        HashSet<AssetId> seen;
        seen.Reserve(dependencies.Len());

        for (const AssetId& dep : dependencies)
        {
            if (dep == id)
            {
                ConsoleLog(
                    ELogLevel::Warning,
                    "SetDependencies: Self-dependency filtered for asset '{}'", id.GetGuid()
                );
                continue;
            }
            if (seen.Insert(dep))
            {
                unique_deps.Push(dep);
            }
        }
    }

    std::unique_lock lock(graph_mutex);

    // 1. 기존 순방향 의존성에서 역방향 인덱스 정리
    if (const auto old_fwd = forward_deps.Find(id))
    {
        for (const AssetId& old_dep : *old_fwd)
        {
            if (const auto rev = reverse_deps.Find(old_dep))
            {
                Array<AssetId>& arr = *rev;

                // 추후 성능상 문제가 생기면 HashSet이나, FlatSet으로 변경
                if (const auto idx = arr.Find(id))
                {
                    arr.RemoveAtSwap(*idx);
                }

                // 비어있으면 역방향 엔트리 제거
                if (arr.IsEmpty())
                {
                    reverse_deps.Remove(old_dep);
                }
            }
        }
        forward_deps.Remove(id);
    }

    // 2. 순환 의존성 필터링 (동일 unique_lock 내에서 원자적으로 수행 - TOCTOU 방지)
    Array<AssetId> safe_deps;
    safe_deps.Reserve(unique_deps.Len());

    for (const AssetId& dep : unique_deps)
    {
        if (HasPathInternal(dep, id))
        {
            ConsoleLog(
                ELogLevel::Warning,
                "SetDependencies: Cyclic dependency '{}' -> '{}' rejected", id.GetGuid(), dep.GetGuid()
            );
            continue;
        }
        safe_deps.Push(dep);
    }

    // 3. 순방향 의존성 등록
    if (safe_deps.IsEmpty())
    {
        return;
    }
    const Array<AssetId>& inserted_deps = forward_deps.Insert(id, std::move(safe_deps));

    // 4. 새 역방향 인덱스 등록
    for (const AssetId& dep : inserted_deps)
    {
        reverse_deps.Entry(dep).OrDefault().Push(id);
    }
}

void DependencyGraph::RemoveNode(const AssetId& id)
{
    ZoneScopedN("DependencyGraph::RemoveNode");
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    std::unique_lock lock(graph_mutex);

    // 1. 순방향 의존성에서 역방향 인덱스 정리
    if (const auto fwd = forward_deps.Find(id))
    {
        for (const AssetId& dep : *fwd)
        {
            if (const auto rev = reverse_deps.Find(dep))
            {
                Array<AssetId>& arr = *rev;

                // 추후 성능상 문제가 생기면 HashSet이나, FlatSet으로 변경
                if (const auto idx = arr.Find(id))
                {
                    arr.RemoveAtSwap(*idx);
                }

                // 비어있으면 역방향 엔트리 제거
                if (arr.IsEmpty())
                {
                    reverse_deps.Remove(dep);
                }
            }
        }
        forward_deps.Remove(id);
    }

    // 2. 역방향 의존성에서 순방향 인덱스 정리 (이 노드에 의존하던 다른 노드들)
    if (const auto rev = reverse_deps.Find(id))
    {
        if (!rev->IsEmpty())
        {
            // 나를 참조하는 대상이 있다면 경고
            ConsoleLog(ELogLevel::Warning, "Asset '{}' is being removed but still has {} dependents!", id.GetGuid(), rev->Len());

            for (const AssetId& dependent : *rev)
            {
                ConsoleLog(ELogLevel::Warning, "  - Dependent: {}", dependent.GetGuid());

                if (const auto fwd = forward_deps.Find(dependent))
                {
                    Array<AssetId>& arr = *fwd;
                    if (const auto idx = arr.Find(id))
                    {
                        arr.RemoveAtSwap(*idx);
                    }

                    // 비어있으면 역방향 엔트리 제거
                    if (arr.IsEmpty())
                    {
                        forward_deps.Remove(dependent);
                    }
                }
            }
        }
        reverse_deps.Remove(id);
    }
}

void DependencyGraph::Clear()
{
    std::unique_lock lock(graph_mutex);
    forward_deps.Clear();
    reverse_deps.Clear();
}

Array<AssetId> DependencyGraph::GetDependencies(const AssetId& id) const
{
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    std::shared_lock lock(graph_mutex);
    return forward_deps.Find(id).Copy().ValueOrDefault();
}

Array<AssetId> DependencyGraph::GetDependents(const AssetId& id) const
{
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    std::shared_lock lock(graph_mutex);
    return reverse_deps.Find(id).Copy().ValueOrDefault();
}

Array<AssetId> DependencyGraph::GetTransitiveDependents(const AssetId& id) const
{
    ZoneScopedN("DependencyGraph::GetTransitiveDependents");
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    std::shared_lock lock(graph_mutex);

    Array<AssetId> result;
    HashSet<AssetId> visited;

    // BFS 큐 (역방향 전파)
    // 초기 타겟의 역방향 의존성들을 먼저 result에 추가
    if (const auto rev = reverse_deps.Find(id))
    {
        // 예상 의존성 개수만큼 예약
        const usize initial_guess = rev->Len() * 2;

        visited.Reserve(initial_guess + 1);
        result.Reserve(initial_guess);

        visited.Insert(id);
        for (const AssetId& dependent : *rev)
        {
            visited.Insert(dependent);
            result.Push(dependent);
        }
    }

    // result 배열 자체를 큐(Queue)로 사용하며 전진
    usize read_idx = 0;
    while (read_idx < result.Len())
    {
        const AssetId current = result[read_idx++]; // Pop 없이 읽기만 함
        if (const auto rev = reverse_deps.Find(current))
        {
            for (const AssetId& dependent : *rev)
            {
                if (!visited.Contains(dependent))
                {
                    visited.Insert(dependent);
                    result.Push(dependent); // 뒤에 계속 쌓임 (자연스러운 BFS)
                }
            }
        }
    }

    return result;
}

bool DependencyGraph::HasCyclicDependency(
    const AssetId& from,
    const AssetId& to
) const
{
    ZoneScopedN("DependencyGraph::HasCyclicDependency");
    SE_ASSERT(from.IsValid() && to.IsValid(), "Invalid asset ID");

    if (from == to)
    {
        return true;
    }

    std::shared_lock lock(graph_mutex);
    return HasPathInternal(to, from);
}

Array<AssetId> DependencyGraph::TopologicalSort() const
{
    ZoneScopedN("DependencyGraph::TopologicalSort");

    std::shared_lock lock(graph_mutex);

    // 1. 모든 노드 수집
    HashMap<AssetId, uint32> dependency_count;

    // forward_deps와 reverse_deps의 Key가 곧 전체 노드 집합
    for (const auto& [node, deps] : forward_deps)
    {
        // node가 의존하는 다른 노드의 수 (forward_deps의 크기)
        dependency_count.Insert(node, static_cast<uint32>(deps.Len()));
    }
    for (const AssetId& node : reverse_deps | std::views::keys)
    {
        if (!dependency_count.Contains(node))
        {
            dependency_count.Insert(node, 0);
        }
    }

    // 2. result 배열을 큐(Queue) 겸 최종 반환 배열로 사용
    Array<AssetId> result;
    result.Reserve(dependency_count.Len());

    // 의존성이 없는 노드부터 탐색
    for (const auto& [node, count] : dependency_count)
    {
        if (count == 0)
        {
            result.Push(node);
        }
    }

    // 3. Index-based BFS 진행
    usize read_idx = 0;
    while (read_idx < result.Len())
    {
        const AssetId current = result[read_idx++];

        // 나를 의존하던 노드(Dependents)의 미해결 의존성 수를 1 감소
        if (const auto rev = reverse_deps.Find(current))
        {
            for (const AssetId& dependent : *rev)
            {
                uint32& deg = dependency_count.FindChecked(dependent);
                SE_ASSERT(deg > 0);

                // 모든 의존성이 해결되면 결과 배열 끝에 추가 (다음 탐색 대상)
                if (--deg == 0)
                {
                    result.Push(dependent);
                }
            }
        }
    }

    // 4. 순환(Cycle) 검증
    // 순환이 있으면 특정 노드의 dependency_count가 0이 되지 못해 result에 포함되지 않음
    if (result.Len() != dependency_count.Len())
    {
        ConsoleLog(ELogLevel::Error, "Cyclic dependency detected!");
        return {};
    }

    return result;
}

uint32 DependencyGraph::GetNodeCount() const
{
    std::shared_lock lock(graph_mutex);

    // |A| + |B|
    const uint32 forward_count = static_cast<uint32>(forward_deps.Len());
    const uint32 reverse_count = static_cast<uint32>(reverse_deps.Len());
    const uint32 total_keys = forward_count + reverse_count;

    // |A ∩ B| (두개 중 더 적은 Map을 순회)
    const auto& [smaller, larger] = forward_count < reverse_count
                                        ? std::tie(forward_deps, reverse_deps)
                                        : std::tie(reverse_deps, forward_deps);

    const uint32 intersection_count = static_cast<uint32>(std::ranges::count_if(smaller | std::views::keys, [&](const auto& key)
    {
        return larger.Contains(key);
    }));

    // |A ∪ B| = |A| + |B| - |A ∩ B|
    return total_keys - intersection_count;
}

bool DependencyGraph::HasPathInternal(
    const AssetId& from,
    const AssetId& target
) const
{
    ZoneScopedN("DependencyGraph::HasPathInternal");

    // target에서 시작해서 from을 만날 수 있는지 확인 (Forward 탐색)
    HashSet<AssetId> visited;
    Array<AssetId> queue;

    // BFS 초기 세팅
    if (const auto fwd = forward_deps.Find(from))
    {
        // 예상 의존성 개수만큼 예약
        const usize initial_guess = fwd->Len() * 2;
        visited.Reserve(initial_guess);
        queue.Reserve(initial_guess);

        visited.Insert(from);
        for (const AssetId& dep : *fwd)
        {
            if (dep == target)
            {
                return true;
            }
            visited.Insert(dep);
            queue.Push(dep);
        }
    }

    usize read_idx = 0;
    while (read_idx < queue.Len())
    {
        const AssetId& current = queue[read_idx++];
        if (const auto fwd = forward_deps.Find(current))
        {
            for (const AssetId& dep : *fwd)
            {
                if (dep == target)
                {
                    return true;
                }

                if (!visited.Contains(dep))
                {
                    visited.Insert(dep);
                    queue.Push(dep);
                }
            }
        }
    }

    return false;
}
} // namespace se::editor
