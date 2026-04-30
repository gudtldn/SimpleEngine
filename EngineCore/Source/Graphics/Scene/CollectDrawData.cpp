#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/MaterialComponent.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include <algorithm>


namespace se::graphics
{

namespace
{
/** DrawCommand의 정렬 키를 계산합니다. */
[[nodiscard]] uint64 ComputeSortKey(const asset::AssetId& mesh_id, const asset::AssetId& material_id)
{
    const uint64 mesh_hash = std::hash<asset::AssetId>{}(mesh_id);
    const uint64 material_hash = std::hash<asset::AssetId>{}(material_id);

    // 상위 32비트: material (PSO/셰이더 변경 최소화)
    // 하위 32비트: mesh     (VB/IB 바인딩 변경 최소화)
    return (material_hash << 32) | (mesh_hash & 0xFFFF'FFFF);
}
} // namespace


SceneDrawData CollectDrawData(const World& world)
{
    SceneDrawData result;

    const asset::AssetSubsystem* asset_subsystem = se::GetSubsystem<const asset::AssetSubsystem>();
    if (!asset_subsystem)
    {
        return result;
    }

    const auto query = world.CreateQuery<
        Entity, const GlobalTransformComponent&, const StaticMeshComponent&, Optional<const MaterialHandleComponent&>
    >();
    for (const auto [entity, global_transform, mesh, material_opt] : query)
    {
        const asset::AssetHandle<asset::StaticMesh> mesh_asset = asset_subsystem->Find<asset::StaticMesh>(mesh.mesh_id);
        if (!mesh_asset)
        {
            continue;
        }

        if (mesh_asset->sections.IsEmpty())
        {
            continue;
        }

        for (uint32 si = 0; si < mesh_asset->sections.Len(); ++si)
        {
            const graphics::MeshSection& section = mesh_asset->sections[si];

            asset::AssetId mat_id = asset::BuiltinAssetIds::DefaultLitInstance;

            if (material_opt && si < material_opt->material_override_ids.Len() && material_opt->material_override_ids[si].IsValid())
            {
                mat_id = material_opt->material_override_ids[si];
            }
            else if (si < mesh_asset->materials.Len() && mesh_asset->materials[si].IsValid())
            {
                mat_id = mesh_asset->materials[si];
            }

            result.opaque_commands.Push({
                .model_matrix          = global_transform.value,
                .entity_id             = entity.GetId(),
                .mesh_id               = mesh.mesh_id,
                .material_id           = mat_id,
                .sort_key              = ComputeSortKey(mesh.mesh_id, mat_id),
                .section_first_index   = section.index_offset,
                .section_index_count   = section.index_count,
                .section_vertex_offset = static_cast<int32>(section.vertex_offset),
            });
        }
    }

    std::ranges::sort(result.opaque_commands, {}, &DrawCommand::sort_key);

    return result;
}
} // namespace se::graphics
