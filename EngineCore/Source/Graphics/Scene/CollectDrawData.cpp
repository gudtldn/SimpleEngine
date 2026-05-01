#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
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

SceneDrawData CollectDrawData(const World& world, const AssetSubsystem& asset_subsystem)
{
    SceneDrawData result;
    FrameMaterialCache& cache = result.material_cache;

    const auto query = world.CreateQuery<
        Entity, const GlobalTransformComponent&, const StaticMeshComponent&, Optional<const MeshMaterialComponent&>
    >();
    for (const auto [entity, global_transform, mesh_comp, material_opt] : query)
    {
        // 1. Asset Subsystem에서 StaticMesh 에셋 핸들 획득
        AssetHandle<StaticMesh> mesh_handle = asset_subsystem.Find<StaticMesh>(mesh_comp.mesh_id);
        if (!mesh_handle)
        {
            // 에셋이 로드되지 않았음 -> 렌더 스킵하고 지연 로드(Lazy Loading) 요청 큐에 등록
            result.requested_meshes.Push(mesh_comp.mesh_id);
            continue; 
        }

        // 2. 에셋 수명 연장 (Pinning)
        // 렌더 스레드가 비동기 렌더링을 마칠 때까지 이 에셋이 메모리에서 Evict되지 않도록 보장합니다.
        result.pinned_meshes.Insert(mesh_comp.mesh_id, mesh_handle);

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

            // 머티리얼 인스턴스 결정 로직
            const AssetId target_mat_id = [&]
            {
                // 오버라이드가 있으면 우선 사용
                if (material_opt.HasValue() && section.material_slot < material_opt->material_overrides.Len())
                {
                    if (const AssetId override_id = material_opt->material_overrides[section.material_slot])
                    {
                        return override_id;
                    }
                }

                // 오버라이드가 없고 메시에 기본 머티리얼이 있으면 사용
                else if (section.material_slot < mesh_handle->default_materials.Len())
                {
                    if (const AssetId default_id = mesh_handle->default_materials[section.material_slot])
                    {
                        return default_id;
                    }
                }

                return BuiltinAssetIds::DefaultLitInstance;
            }();

            // 아레나 패킹 및 캐싱
            if (const auto slot_idx = cache.material_to_slot.Find(target_mat_id))
            {
                cmd.material_slot_index = *slot_idx; // 이미 이번 프레임에 처리된 머티리얼이면 캐시 히트
            }
            else
            {
                // 캐시에 없으면 에셋을 찾아 UBO를 패킹
                AssetHandle<MaterialInstance> inst_handle = asset_subsystem.Find<MaterialInstance>(target_mat_id);
                if (!inst_handle)
                {
                    inst_handle = asset_subsystem.Find<MaterialInstance>(BuiltinAssetIds::DefaultLitInstance);
                }

                if (inst_handle)
                {
                    if (AssetHandle<Material> mat_handle = asset_subsystem.Find<Material>(inst_handle->parent_material_id))
                    {
                        // 에셋 Pinning
                        result.pinned_materials.Insert(mat_handle.GetAssetId(), mat_handle);
                        result.pinned_material_instances.Insert(inst_handle.GetAssetId(), inst_handle);

                        FrameMaterialCache::MaterialSlot slot;

                        // UBO 팩킹
                        slot.ubo_offset = static_cast<uint32>(cache.ubo_arena.Len());
                        slot.ubo_size = static_cast<uint16>(inst_handle->parameter_values.Len());
                        cache.ubo_arena.PushRange(inst_handle->parameter_values);

                        // 텍스처 바인딩 팩킹
                        slot.binding_offset = static_cast<uint32>(cache.binding_arena.Len());
                        for (const MaterialTextureSlot& tex_slot : mat_handle->texture_slots)
                        {
                            cache.binding_arena.Push({
                                .fragment_slot = tex_slot.fragment_slot,
                                .texture_id = inst_handle->GetTextureOrDefault(tex_slot.name, *mat_handle),
                                .sampler = tex_slot.sampler,
                            });
                        }
                        slot.binding_count = static_cast<uint16>(cache.binding_arena.Len() - slot.binding_offset);

                        // 슬롯 등록
                        const uint16 slot_index = static_cast<uint16>(cache.slots.Len());
                        cache.slots.Push(slot);
                        cache.material_to_slot.Insert(target_mat_id, slot_index);

                        cmd.material_slot_index = slot_index;
                    }
                }
            }

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
