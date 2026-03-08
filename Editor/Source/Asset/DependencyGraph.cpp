#include "SimpleEditor/Asset/DependencyGraph.h"

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Debug.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
void DependencyGraph::SetDependencies(
    const asset::AssetId& id,
    ArrayView<const asset::AssetId> dependencies
)
{
    ZoneScopedN("DependencyGraph::SetDependencies");
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    std::unique_lock lock(graph_mutex);

    // 1) 기존 순방향 의존성에서 역방향 인덱스 정리
    if (const auto old_fwd = forward_deps.Find(id))
    {
        for (const asset::AssetId& old_dep : *old_fwd)
        {
            if (const auto rev = reverse_deps.Find(old_dep))
            {
                Array<asset::AssetId>& arr = *rev;
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
    }

    // 2) 순방향 의존성 교체
    if (dependencies.IsEmpty())
    {
        forward_deps.Remove(id);
        return;
    }
    forward_deps.Insert(id, Array<asset::AssetId>::FromRange(dependencies));

    // 3) 새 역방향 인덱스 등록
    for (const asset::AssetId& dep : dependencies)
    {
        SE_ASSERT(id != dep, "Self-dependency detected");
        reverse_deps.Entry(dep).OrDefault().Push(id);
    }
}

void DependencyGraph::RemoveNode(const asset::AssetId& id)
{
    ZoneScopedN("DependencyGraph::RemoveNode");
    SE_ASSERT(id.IsValid(), "Invalid asset ID");

    std::unique_lock lock(graph_mutex);

    // 1) 순방향 의존성에서 역방향 인덱스 정리
    if (const auto fwd = forward_deps.Find(id))
    {
        for (const asset::AssetId& dep : *fwd)
        {
            if (const auto rev = reverse_deps.Find(dep))
            {
                Array<asset::AssetId>& arr = *rev;
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

    // 2) 역방향 의존성에서 순방향 인덱스 정리 (이 노드에 의존하던 다른 노드들)
    if (const auto rev = reverse_deps.Find(id))
    {
        // 나를 참조하는 대상이 있다면 런타임 경고 발생
        if (!rev->IsEmpty())
        {
            // 나를 참조하는 대상이 있다면 경고
            ConsoleLog(ELogLevel::Warning, "Asset '{}' is being removed but still has {} dependents!", id.GetGuid(), rev->Len());

            for (const asset::AssetId& dependent : *rev)
            {
                if (const auto fwd = forward_deps.Find(dependent))
                {
                    Array<asset::AssetId>& arr = *fwd;
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

Array<asset::AssetId> DependencyGraph::GetDependencies(const asset::AssetId& id) const
{
    std::shared_lock lock(graph_mutex);
    return forward_deps.Find(id).Copy().ValueOrDefault();
}

Array<asset::AssetId> DependencyGraph::GetDependents(const asset::AssetId& id) const
{
    std::shared_lock lock(graph_mutex);
    return reverse_deps.Find(id).Copy().ValueOrDefault();
}

Array<asset::AssetId> DependencyGraph::GetTransitiveDependents(const asset::AssetId& id) const
{
    ZoneScopedN("DependencyGraph::GetTransitiveDependents");

    std::shared_lock lock(graph_mutex);

    Array<asset::AssetId> result;
    HashSet<asset::AssetId> visited;

    // BFS 큐 (역방향 전파)
    // 초기 타겟의 역방향 의존성들을 먼저 result에 추가
    if (const auto rev = reverse_deps.Find(id))
    {
        // 예상 의존성 개수만큼 예약
        const usize initial_guess = rev->Len() * 2;

        visited.Reserve(initial_guess + 1);
        result.Reserve(initial_guess);

        visited.Insert(id);
        for (const asset::AssetId& dependent : *rev)
        {
            visited.Insert(dependent);
            result.Push(dependent);
        }
    }

    // result 배열 자체를 큐(Queue)로 사용하며 전진
    usize read_idx = 0;
    while (read_idx < result.Len())
    {
        const asset::AssetId current = result[read_idx++]; // Pop 없이 읽기만 함
        if (const auto rev = reverse_deps.Find(current))
        {
            for (const asset::AssetId& dependent : *rev)
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
    const asset::AssetId& from,
    const asset::AssetId& to
) const
{
    ZoneScopedN("DependencyGraph::HasCyclicDependency");

    if (from == to)
    {
        return true;
    }

    std::shared_lock lock(graph_mutex);

    // to에서 시작해서 from을 만날 수 있는지 확인 (Forward 탐색)
    HashSet<asset::AssetId> visited;
    Array<asset::AssetId> queue;

    if (const auto fwd = forward_deps.Find(to))
    {
        // 예상 의존성 개수만큼 예약
        const usize initial_guess = fwd->Len() * 2;

        visited.Reserve(initial_guess);
        queue.Reserve(initial_guess);

        visited.Insert(to);
        for (const asset::AssetId& dep : *fwd)
        {
            if (dep == from)
            {
                return true; // 순환 의존성 감지
            }
            visited.Insert(dep);
            queue.Push(dep);
        }
    }

    usize read_idx = 0;
    while (read_idx < queue.Len())
    {
        const asset::AssetId current = queue[read_idx++];
        if (const auto fwd = forward_deps.Find(current))
        {
            for (const asset::AssetId& dep : *fwd)
            {
                if (dep == from)
                {
                    return true; // 순환 의존성 감지
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

Array<asset::AssetId> DependencyGraph::TopologicalSort() const
{
    ZoneScopedN("DependencyGraph::TopologicalSort");

    std::shared_lock lock(graph_mutex);

    // 1) 모든 노드 수집
    HashMap<asset::AssetId, uint32> in_degree;

    // forward_deps와 reverse_deps의 Key가 곧 전체 노드 집합
    for (const auto& [node, deps] : forward_deps)
    {
        // forward_deps의 크기가 곧 그 노드의 In-degree (의존하는 개수)
        in_degree.Insert(node, static_cast<uint32>(deps.Len()));
    }
    for (const asset::AssetId& node : reverse_deps | std::views::keys)
    {
        if (!in_degree.Contains(node))
        {
            in_degree.Insert(node, 0);
        }
    }

    // 2) result 배열을 큐(Queue) 겸 최종 반환 배열로 사용
    Array<asset::AssetId> result;
    result.Reserve(in_degree.Len());

    // 의존성이 없는 노드부터 탐색
    for (const auto& [node, degree] : in_degree)
    {
        if (degree == 0)
        {
            result.Push(node);
        }
    }

    // 3) Index-based BFS 진행
    usize read_idx = 0;
    while (read_idx < result.Len())
    {
        const asset::AssetId current = result[read_idx++];

        // 나를 의존하던 노드(Dependents)의 In-degree를 하나씩 감소
        if (const auto rev = reverse_deps.Find(current))
        {
            for (const asset::AssetId& dependent : *rev)
            {
                uint32& deg = in_degree.FindChecked(dependent);
                SE_ASSERT(deg > 0);

                // 의존성이 모두 해결되면 결과 배열 끝에 추가 (다음 탐색 대상)
                if (--deg == 0)
                {
                    result.Push(dependent);
                }
            }
        }
    }

    // 4) 순환(Cycle) 검증
    // 순환이 발생했다면 특정 노드들의 in_degree가 0이 되지 못해 result에 들어오지 못함
    if (result.Len() != in_degree.Len())
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

    const uint32 intersection_count = std::ranges::count_if(smaller | std::views::keys, [&](const auto& key)
    {
        return larger.Contains(key);
    });

    // |A ∪ B| = |A| + |B| - |A ∩ B|
    return total_keys - intersection_count;
}
} // namespace se::editor
