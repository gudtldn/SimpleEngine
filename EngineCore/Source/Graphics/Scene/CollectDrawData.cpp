#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/MaterialComponent.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"

#include <algorithm>


namespace se
{
namespace
{
/**
 * DrawCommand의 정렬 키를 계산합니다.
 * @todo 나중에 MDI(Multi-Draw Indirect) 렌더링 및 투명/불투명 분류 시, 머티리얼 PSO 해시 및 뎁스(Z) 값을 조합해 정렬 키를 계산해야 함.
 */
[[nodiscard]] uint64 ComputeSortKey(const AssetId& mesh_id)
{
    const uint64 mesh_hash = std::hash<AssetId>{}(mesh_id);
    return mesh_hash;
}
} // namespace

SceneDrawData CollectDrawData(const World& world, AssetSubsystem& asset_subsystem)
{
    SceneDrawData result;

    const auto query = world.CreateQuery<
        Entity, const GlobalTransformComponent&, const StaticMeshComponent&, Optional<const MeshMaterialComponent&>
    >();
    for (const auto [entity, global_transform, mesh_comp, material_opt] : query)
    {
        // 1. Asset Subsystem에서 StaticMesh 에셋 핸들 획득
        AssetHandle<StaticMesh> mesh_handle = asset_subsystem.Find<StaticMesh>(mesh_comp.mesh_id);
        if (!mesh_handle)
        {
            continue; // 에셋이 로드되지 않았거나 유효하지 않음
        }

        // 2. 에셋 수명 연장 (Pinning)
        // 렌더 스레드가 비동기 렌더링을 마칠 때까지 이 에셋이 메모리에서 Evict되지 않도록 보장합니다.
        result.pinned_meshes.Push(mesh_handle);

        // 메시에 LOD가 없으면 렌더링 스킵
        if (mesh_handle->lods.IsEmpty())
        {
            continue;
        }

        // Mesh LOD 가져오기
        const MeshLOD& target_lod = [&] -> const MeshLOD&
        {
            const isize target_lod_index = std::min<isize>(mesh_comp.force_lod, static_cast<isize>(mesh_handle->lods.Len() - 1));
            if (target_lod_index > -1)
            {
                return mesh_handle->lods[target_lod_index];
            }

            // TODO: 카메라 거리에 따른 LOD 동적 계산 로직 추가
            // 현재는 강제 오버라이드(force_lod)가 설정되어 있으면 해당 인덱스를, 아니면 기본값(0)을 사용합니다.
            return mesh_handle->lods[0];
        }();

        // 3. 머티리얼 오버라이드 획득
        (void)material_opt; // 경고 방지

        // 4. 서브메시(Section) 단위로 쪼개서 DrawCommand 생성
        for (const MeshSection& section : target_lod.sections)
        {
            // TODO: 프러스텀 컬링(Frustum Culling)은 여기서 section.bounds와 transform을 이용해서 수행해야 함.

            DrawCommand cmd;
            cmd.model_matrix = global_transform.value;
            cmd.entity_id = entity.GetId();

            // TODO: sort_key에 머티리얼 ID와 뎁스를 조합하여 해시해야 함
            cmd.sort_key = ComputeSortKey(mesh_comp.mesh_id);

            // 향후 제거 예정
            cmd.mesh_id = mesh_comp.mesh_id;

            // TODO: 머티리얼 슬롯(Material Slot) 처리
            // section.material_slot을 인덱스로 사용하여, mat_comp->material_overrides를 우선 확인하고,
            // 없다면 static_mesh->default_materials를 사용해야 합니다.
            // 이후 결정된 AssetId로 머티리얼 에셋을 Find()하여 pinned_materials에 넣는 로직이 필요합니다.

            // MDI Command 구조체 설정
            cmd.draw_params.index_count = section.index_count;
            cmd.draw_params.instance_count = 1;
            cmd.draw_params.first_index = section.index_offset;
            cmd.draw_params.vertex_offset = section.vertex_offset;
            cmd.draw_params.first_instance = 0; // 크로스플랫폼 셰이더 호환성을 위해 0으로 고정

            result.opaque_commands.Push(cmd);
        }
    }

    std::ranges::sort(result.opaque_commands, {}, &DrawCommand::sort_key);

    return result;
}
} // namespace se
