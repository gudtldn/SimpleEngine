#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/MeshMaterialComponent.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"

#include <algorithm>


namespace se
{
namespace
{
/** 정렬 키에 포함될 개별 데이터 필드 */
enum class SortField : uint8
{
    Mesh,     // 메시 버퍼 해시 (VB/IB 바인딩 변경 최소화)
    Material, // 머티리얼 인스턴스 해시 (descriptor 바인딩 변경 최소화)
    Pso,      // 파이프라인 스테이트 해시 (PSO 전환 최소화)
    Depth,    // 카메라 거리 기반 깊이 (불투명: 앞 -> 뒤 정렬로 overdraw 감소)
    Layer,    // 0=Opaque, 1=Transparent (투명 오브젝트를 불투명 이후로 밀어냄)
};

/** 레이아웃 정의를 위한 구조체 */
struct FieldLayout
{
    SortField id;
    uint64 bits;
};

/** 비트 연산을 위한 시프트/마스크 정보 구조체 */
struct FieldBitInfo
{
    uint64 shift;
    uint64 mask;
};

/**
 * sort_key 64비트 레이아웃 (단일 진실 공급원)
 * 하위 비트부터 누적 배치되며, 상위 비트에 위치할수록 정렬 우선순위가 높습니다.
 */
constexpr FieldLayout LAYOUT[] = {
    { .id = SortField::Mesh,     .bits = 16 },
    { .id = SortField::Material, .bits = 16 },
    { .id = SortField::Pso,      .bits = 16 },
    { .id = SortField::Depth,    .bits = 15 },
    { .id = SortField::Layer,    .bits = 1  },
};

// 64비트 총합 검증
static_assert(
    std::ranges::fold_left(LAYOUT | std::views::transform(&FieldLayout::bits), 0ULL, std::plus<>{}) == 64,
    "SortKey layout MUST be exactly 64 bits!"
);

/**
 * 대상 필드의 shift와 mask 값을 동시에 계산하여 반환합니다.
 * @return FieldBitInfo (shift 위치, 해당 비트 폭에 맞는 mask)
 */
template <SortField Target>
consteval FieldBitInfo GetFieldInfo()
{
    uint64 shift = 0;
    for (const auto& [id, bits] : LAYOUT)
    {
        if (id == Target)
        {
            return {
                .shift = shift,
                .mask = (1ULL << bits) - 1
            };
        }
        shift += bits;
    }
    throw "Invalid SortField"; // NOLINT(*-exception-baseclass)
}

/**
 * 주어진 값을 대상 필드의 비트 레이아웃에 맞게 안전하게 패킹합니다.
 */
template <SortField Target>
constexpr uint64 PackField(uint64 value)
{
    constexpr FieldBitInfo info = GetFieldInfo<Target>();
    return (value & info.mask) << info.shift;
}

/**
 * 렌더링 큐 제출을 위한 64비트 정렬 키를 생성합니다.
 * @todo 향후 PSO, Depth, Layer 파라미터 추가 시 조합에 포함해야 합니다.
 */
[[nodiscard]] uint64 ComputeSortKey(const AssetId& mesh_id, const AssetId& material_instance_id, const AssetId& parent_material_id, uint8 layer = 0)
{
    const uint64 parent_hash = std::hash<AssetId>{}(parent_material_id);
    const uint64 inst_hash   = std::hash<AssetId>{}(material_instance_id);
    const uint64 mesh_hash   = std::hash<AssetId>{}(mesh_id);

    return PackField<SortField::Layer>(layer)
         | PackField<SortField::Pso>(parent_hash)
         | PackField<SortField::Material>(inst_hash)
         | PackField<SortField::Mesh>(mesh_hash);
}
} // namespace

SceneDrawData CollectDrawData(const World& world, const AssetSubsystem& asset_subsystem, const GpuResourceManager& gpu_manager)
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

        // GPU 슬라이스 Pre-resolve: 아직 GPU에 업로드되지 않은 메시는 이번 프레임 스킵 (1-frame 레이턴시 허용)
        const Optional<const GpuBufferSlice&> gpu_slice = gpu_manager.GetSlice(mesh_comp.mesh_id);
        if (!gpu_slice.HasValue())
        {
            continue;
        }

        // 4. 서브메시(Section) 단위로 쪼개서 DrawCommand 생성
        for (const MeshSection& section : target_lod.sections)
        {
            // TODO: 프러스텀 컬링(Frustum Culling)은 여기서 section.bounds와 transform을 이용해서 수행해야 함.

            DrawCommand cmd;
            cmd.model_matrix = global_transform.value;
            cmd.entity_id = entity.GetId();

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
                    if (target_mat_id != BuiltinAssetIds::DefaultLitInstance)
                    {
                        result.requested_material_instances.Push(target_mat_id);
                    }
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
                        slot.parent_material_id = mat_handle.GetAssetId();

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

            // DrawCommand Sort Key 계산
            {
                const FrameMaterialCache::MaterialSlot& slot = cache.slots[cmd.material_slot_index];
                cmd.material_instance_id = target_mat_id;
                cmd.sort_key = ComputeSortKey(mesh_comp.mesh_id, target_mat_id, slot.parent_material_id);
            }

            // GPU 버퍼 필드 채우기
            cmd.gpu_buffer = gpu_slice->buffer;
            cmd.vertex_buffer_offset = gpu_slice->offset;
            cmd.index_buffer_offset = gpu_slice->index_offset;
            cmd.vertex_count = section.vertex_count;

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
