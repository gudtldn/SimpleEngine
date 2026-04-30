#include "gtest/gtest.h"

#include "SimpleEditor/Asset/DependencyGraph.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Types/Guid.h"

using namespace se;
using namespace se::editor;


namespace
{
AssetId NewId() { return AssetId(Guid::NewGuid()); }

/** 배열에 특정 ID가 포함되어 있는지 확인하는 헬퍼 */
bool Contains(const Array<AssetId>& arr, const AssetId& id)
{
    return std::ranges::any_of(arr, [&](const AssetId& item)
    {
        return item == id;
    });
}

/** 배열을 HashSet으로 변환하는 헬퍼 */
HashSet<AssetId> ToSet(const Array<AssetId>& arr)
{
    HashSet<AssetId> result;
    for (const auto& item : arr)
    {
        result.Insert(item);
    }
    return result;
}
} // namespace


// ── 기본 등록/조회 ─────────────────────────────────────────────────

TEST(DependencyGraph, SetDependencies_ForwardQuery)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    Array<AssetId> deps;
    deps.Push(b);
    deps.Push(c);
    graph.SetDependencies(a, deps);

    const auto fwd = graph.GetDependencies(a);
    EXPECT_EQ(fwd.Len(), 2);
    EXPECT_TRUE(Contains(fwd, b));
    EXPECT_TRUE(Contains(fwd, c));
}

TEST(DependencyGraph, SetDependencies_ReverseQuery)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A depends on B, C
    Array<AssetId> deps;
    deps.Push(b);
    deps.Push(c);
    graph.SetDependencies(a, deps);

    // B의 역참조에 A가 있어야 함
    const auto rev_b = graph.GetDependents(b);
    EXPECT_EQ(rev_b.Len(), 1);
    EXPECT_TRUE(Contains(rev_b, a));

    // C의 역참조에 A가 있어야 함
    const auto rev_c = graph.GetDependents(c);
    EXPECT_EQ(rev_c.Len(), 1);
    EXPECT_TRUE(Contains(rev_c, a));
}

TEST(DependencyGraph, SetDependencies_ReplacesOldDeps)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A -> {B}
    Array<AssetId> deps1;
    deps1.Push(b);
    graph.SetDependencies(a, deps1);

    // A -> {C} (B 교체)
    Array<AssetId> deps2;
    deps2.Push(c);
    graph.SetDependencies(a, deps2);

    // A의 순방향은 C만
    const auto fwd = graph.GetDependencies(a);
    EXPECT_EQ(fwd.Len(), 1);
    EXPECT_TRUE(Contains(fwd, c));

    // B의 역참조에서 A 제거됨
    const auto rev_b = graph.GetDependents(b);
    EXPECT_TRUE(rev_b.IsEmpty());

    // C의 역참조에 A 등록됨
    const auto rev_c = graph.GetDependents(c);
    EXPECT_EQ(rev_c.Len(), 1);
    EXPECT_TRUE(Contains(rev_c, a));
}

TEST(DependencyGraph, EmptyDependencies_RemovesForward)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();

    Array<AssetId> deps;
    deps.Push(b);
    graph.SetDependencies(a, deps);

    // 빈 배열로 의존성 제거
    graph.SetDependencies(a, {});

    EXPECT_TRUE(graph.GetDependencies(a).IsEmpty());
    EXPECT_TRUE(graph.GetDependents(b).IsEmpty());
}

// ── 노드 제거 ──────────────────────────────────────────────────────

TEST(DependencyGraph, RemoveNode_CleansForwardAndReverse)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A -> {B}, C -> {B}
    Array<AssetId> deps_a;
    deps_a.Push(b);
    graph.SetDependencies(a, deps_a);

    Array<AssetId> deps_c;
    deps_c.Push(b);
    graph.SetDependencies(c, deps_c);

    // B 제거 -> A, C의 순방향에서 B 사라져야 함
    graph.RemoveNode(b);

    EXPECT_TRUE(graph.GetDependencies(a).IsEmpty());
    EXPECT_TRUE(graph.GetDependencies(c).IsEmpty());
    EXPECT_TRUE(graph.GetDependents(b).IsEmpty());
}

TEST(DependencyGraph, RemoveNode_Nonexistent_NoOp)
{
    DependencyGraph graph;
    graph.RemoveNode(NewId());  // 존재하지 않는 노드 제거 — 크래시 없어야 함
}

// ── 전이적 역참조 (Transitive Dependents) ───────────────────────────

TEST(DependencyGraph, GetTransitiveDependents_ChainPropagation)
{
    DependencyGraph graph;
    const AssetId texture = NewId();
    const AssetId material = NewId();
    const AssetId mesh = NewId();

    // mesh -> material -> texture
    Array<AssetId> mat_deps;
    mat_deps.Push(texture);
    graph.SetDependencies(material, mat_deps);

    Array<AssetId> mesh_deps;
    mesh_deps.Push(material);
    graph.SetDependencies(mesh, mesh_deps);

    // texture 변경 시 -> material, mesh 전파
    const auto trans = graph.GetTransitiveDependents(texture);
    EXPECT_EQ(trans.Len(), 2);
    EXPECT_TRUE(Contains(trans, material));
    EXPECT_TRUE(Contains(trans, mesh));
}

TEST(DependencyGraph, GetTransitiveDependents_Diamond)
{
    //   A
    //  / \
    // B   C
    //  \ /
    //   D
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();
    const AssetId d = NewId();

    // B -> A, C -> A, D -> {B, C}
    Array<AssetId> b_deps;
    b_deps.Push(a);
    graph.SetDependencies(b, b_deps);

    Array<AssetId> c_deps;
    c_deps.Push(a);
    graph.SetDependencies(c, c_deps);

    Array<AssetId> d_deps;
    d_deps.Push(b);
    d_deps.Push(c);
    graph.SetDependencies(d, d_deps);

    // A 변경 시 -> B, C, D
    auto trans = graph.GetTransitiveDependents(a);
    EXPECT_EQ(trans.Len(), 3);
    auto trans_set = ToSet(trans);
    EXPECT_TRUE(trans_set.Contains(b));
    EXPECT_TRUE(trans_set.Contains(c));
    EXPECT_TRUE(trans_set.Contains(d));
}

TEST(DependencyGraph, GetTransitiveDependents_NoTransitiveDeps)
{
    DependencyGraph graph;
    const AssetId a = NewId();

    auto trans = graph.GetTransitiveDependents(a);
    EXPECT_TRUE(trans.IsEmpty());
}

// ── 순환 감지 ──────────────────────────────────────────────────────

TEST(DependencyGraph, HasCyclicDependency_SelfLoop)
{
    DependencyGraph graph;
    const AssetId a = NewId();

    EXPECT_TRUE(graph.HasCyclicDependency(a, a));
}

TEST(DependencyGraph, HasCyclicDependency_DirectCycle)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();

    // A -> B 설정 후, B -> A 추가하면 순환
    Array<AssetId> a_deps;
    a_deps.Push(b);
    graph.SetDependencies(a, a_deps);

    EXPECT_TRUE(graph.HasCyclicDependency(b, a));
}

TEST(DependencyGraph, HasCyclicDependency_IndirectCycle)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A -> B -> C, 그 상태에서 C -> A 추가하면 순환
    Array<AssetId> a_deps;
    a_deps.Push(b);
    graph.SetDependencies(a, a_deps);

    Array<AssetId> b_deps;
    b_deps.Push(c);
    graph.SetDependencies(b, b_deps);

    EXPECT_TRUE(graph.HasCyclicDependency(c, a));
}

TEST(DependencyGraph, HasCyclicDependency_NoCycle)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A -> B
    Array<AssetId> a_deps;
    a_deps.Push(b);
    graph.SetDependencies(a, a_deps);

    // C -> A 는 순환이 아님
    EXPECT_FALSE(graph.HasCyclicDependency(c, a));
}

// ── 위상 정렬 ──────────────────────────────────────────────────────

TEST(DependencyGraph, TopologicalSort_LinearChain)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // C -> B -> A (C depends on B, B depends on A)
    Array<AssetId> b_deps;
    b_deps.Push(a);
    graph.SetDependencies(b, b_deps);

    Array<AssetId> c_deps;
    c_deps.Push(b);
    graph.SetDependencies(c, c_deps);

    auto sorted = graph.TopologicalSort();
    EXPECT_EQ(sorted.Len(), 3);

    // A가 B보다 앞, B가 C보다 앞
    usize idx_a = 0, idx_b = 0, idx_c = 0; // NOLINT(*-isolate-declaration)
    for (usize i = 0; i < sorted.Len(); ++i)
    {
        if (sorted[i] == a) { idx_a = i; }
        if (sorted[i] == b) { idx_b = i; }
        if (sorted[i] == c) { idx_c = i; }
    }
    EXPECT_LT(idx_a, idx_b);
    EXPECT_LT(idx_b, idx_c);
}

TEST(DependencyGraph, TopologicalSort_CyclePreventedBySetDependencies)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();

    // A -> B 설정
    Array<AssetId> a_deps;
    a_deps.Push(b);
    graph.SetDependencies(a, a_deps);

    // B -> A 시도 — SetDependencies 내부에서 순환 감지, A가 거부됨
    Array<AssetId> b_deps;
    b_deps.Push(a);
    graph.SetDependencies(b, b_deps);

    // B의 의존성은 비어있어야 함 (순환 엣지 거부)
    EXPECT_TRUE(graph.GetDependencies(b).IsEmpty());

    // TopologicalSort는 정상 작동 (순환이 그래프에 진입하지 못함)
    auto sorted = graph.TopologicalSort();
    EXPECT_EQ(sorted.Len(), 2);
}

TEST(DependencyGraph, TopologicalSort_EmptyGraph)
{
    DependencyGraph graph;
    auto sorted = graph.TopologicalSort();
    EXPECT_TRUE(sorted.IsEmpty());
}

// ── Clear ──────────────────────────────────────────────────────────

TEST(DependencyGraph, Clear_RemovesAllData)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();

    Array<AssetId> deps;
    deps.Push(b);
    graph.SetDependencies(a, deps);

    graph.Clear();

    EXPECT_EQ(graph.GetNodeCount(), 0);
    EXPECT_TRUE(graph.GetDependencies(a).IsEmpty());
    EXPECT_TRUE(graph.GetDependents(b).IsEmpty());
}

// ── GetNodeCount ───────────────────────────────────────────────────

TEST(DependencyGraph, GetNodeCount_CountsAllUniqueNodes)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A -> {B, C}
    Array<AssetId> deps;
    deps.Push(b);
    deps.Push(c);
    graph.SetDependencies(a, deps);

    // A, B, C 총 3개 노드
    EXPECT_EQ(graph.GetNodeCount(), 3);
}

// ── 중복 입력 방어 ────────────────────────────────────────────────

TEST(DependencyGraph, SetDependencies_DeduplicatesInput)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();

    // 동일한 ID를 여러 번 포함한 의존성 목록
    Array<AssetId> deps;
    deps.Push(b);
    deps.Push(b);
    deps.Push(b);
    graph.SetDependencies(a, deps);

    // 순방향에 중복 없이 1개만 등록되어야 함
    const auto fwd = graph.GetDependencies(a);
    EXPECT_EQ(fwd.Len(), 1);
    EXPECT_TRUE(Contains(fwd, b));

    // 역방향에도 A가 1번만 등록되어야 함
    const auto rev = graph.GetDependents(b);
    EXPECT_EQ(rev.Len(), 1);
    EXPECT_TRUE(Contains(rev, a));
}

TEST(DependencyGraph, SetDependencies_FiltersSelfDependency)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();

    // 자기 자신(A)을 의존성에 포함 — Assert 대신 조용히 필터링되어야 함
    Array<AssetId> deps;
    deps.Push(a);
    deps.Push(b);
    graph.SetDependencies(a, deps);

    // 자기 참조는 제거되고 B만 남아야 함
    const auto fwd = graph.GetDependencies(a);
    EXPECT_EQ(fwd.Len(), 1);
    EXPECT_TRUE(Contains(fwd, b));
}

TEST(DependencyGraph, SetDependencies_RejectsCyclicDependency)
{
    DependencyGraph graph;
    const AssetId a = NewId();
    const AssetId b = NewId();
    const AssetId c = NewId();

    // A -> B -> C 체인 구축
    Array<AssetId> a_deps;
    a_deps.Push(b);
    graph.SetDependencies(a, a_deps);

    Array<AssetId> b_deps;
    b_deps.Push(c);
    graph.SetDependencies(b, b_deps);

    // C -> A 시도 — 간접 순환(A->B->C->A)이므로 거부되어야 함
    Array<AssetId> c_deps;
    c_deps.Push(a);
    graph.SetDependencies(c, c_deps);

    // C의 의존성은 비어있어야 함
    EXPECT_TRUE(graph.GetDependencies(c).IsEmpty());

    // HasCyclicDependency도 여전히 정상 동작 확인
    EXPECT_TRUE(graph.HasCyclicDependency(c, a));
    EXPECT_FALSE(graph.HasCyclicDependency(c, NewId()));
}
