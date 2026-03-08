#pragma once

#include "SimpleEditor/EditorAPI.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/HashMap.h"

#include "tracy/Tracy.hpp"

#include <shared_mutex>


namespace se::editor
{
/**
 * 에셋 간 의존성을 관리하는 인메모리 방향 그래프 (Editor 전용)
 *
 * .meta 파일의 dependencies[] 배열을 "진실의 원천"으로 삼고,
 * ScanWorkspace 시 순방향/역방향 인덱스를 구축합니다.
 */
class SE_EDITOR_API DependencyGraph
{
public:
    DependencyGraph() = default;
    ~DependencyGraph() = default;

    // 복사 & 이동 금지
    DependencyGraph(const DependencyGraph&) = delete;
    DependencyGraph& operator=(const DependencyGraph&) = delete;
    DependencyGraph(DependencyGraph&&) = delete;
    DependencyGraph& operator=(DependencyGraph&&) = delete;

public:
    /**
     * 에셋의 순방향 의존성을 설정합니다. (기존 의존성을 완전 교체)
     *
     * 기존 forward/reverse 인덱스를 정리한 뒤 새 의존성으로 갱신합니다.
     * CookAsset 완료 후 호출됩니다.
     *
     * @param id 의존성을 설정할 에셋의 ID
     * @param dependencies 이 에셋이 의존하는 대상 ID 목록
     */
    void SetDependencies(const asset::AssetId& id, ArrayView<const asset::AssetId> dependencies);

    /**
     * 그래프에서 노드를 완전 제거합니다. (순방향 + 역방향 모두)
     *
     * 에셋 삭제 시 Registry.UnregisterAsset과 함께 호출합니다.
     * @param id 제거할 에셋의 ID
     */
    void RemoveNode(const asset::AssetId& id);

    /** 그래프의 모든 데이터를 초기화합니다. */
    void Clear();

public:
    /**
     * 주어진 에셋이 의존하는(내가 필요로 하는) 에셋 목록을 반환합니다. (순방향)
     * @param id 조회할 에셋의 ID
     * @return 순방향 의존성 목록 (복사본)
     */
    [[nodiscard]] Array<asset::AssetId> GetDependencies(const asset::AssetId& id) const;

    /**
     * 주어진 에셋에 직접 의존하는(다른 에셋이 나를 필요로 하는) 에셋 목록을 반환합니다. (역방향)
     * @param id 조회할 에셋의 ID
     * @return 역방향 의존성 목록 (복사본)
     */
    [[nodiscard]] Array<asset::AssetId> GetDependents(const asset::AssetId& id) const;

    /**
     * 에셋에 직/간접적으로 의존하는 모든 대상을 BFS로 수집합니다.
     *
     * Hot-Reload 시 변경된 에셋으로부터 전파 대상을 찾는 핵심 쿼리입니다.
     * @param id 기준 에셋의 ID
     * @return 전이적 역방향 의존 목록 (id 자신은 포함하지 않음)
     */
    [[nodiscard]] Array<asset::AssetId> GetTransitiveDependents(const asset::AssetId& id) const;

    /**
     * from -> to 의존성을 추가했을 때 순환이 발생하는지 검사합니다.
     * @param from 의존하는 쪽의 에셋 ID
     * @param to 의존 대상 에셋 ID
     * @return 순환이 발생하면 true
     */
    [[nodiscard]] bool HasCyclicDependency(const asset::AssetId& from, const asset::AssetId& to) const;

    /**
     * 전체 그래프의 위상 정렬 결과를 반환합니다. (Kahn's Algorithm)
     *
     * 병렬 Cook 스케줄링에서 의존성 순서대로 Task를 발행할 때 사용합니다.
     * @note 순환이 있으면 빈 배열을 반환합니다.
     * @return 위상 정렬된 AssetId 목록 (의존 대상이 앞에 위치)
     */
    [[nodiscard]] Array<asset::AssetId> TopologicalSort() const;

    /** 그래프에 등록된 노드(에셋) 수를 반환합니다. */
    [[nodiscard]] uint32 GetNodeCount() const;

private:
    /**
     * from에서 target까지 순방향으로 도달 가능한지 BFS로 검사합니다. (내부 전용)
     * @pre graph_mutex를 호출자가 이미 보유해야 합니다. (shared 또는 unique)
     */
    [[nodiscard]] bool HasPathInternal(const asset::AssetId& from, const asset::AssetId& target) const;

    mutable TracySharedLockable(std::shared_mutex, graph_mutex);

    /** 순방향 인덱스: A -> {B, C} (A가 B, C에 의존) */
    HashMap<asset::AssetId, Array<asset::AssetId>> forward_deps;

    /** 역방향 인덱스: B -> {A, D} (A, D가 B에 의존) */
    HashMap<asset::AssetId, Array<asset::AssetId>> reverse_deps;
};
} // namespace se::editor
