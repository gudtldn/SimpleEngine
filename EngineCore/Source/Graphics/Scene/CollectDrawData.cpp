#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/MaterialComponent.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"

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

    const auto query = world.CreateQuery<Entity, const GlobalTransformComponent&, const StaticMeshComponent&, const MaterialHandleComponent&>();
    for (const auto [entity, global_transform, mesh, material] : query)
    {
        result.opaque_commands.Push({
            .model_matrix = global_transform.value,
            .entity_id = entity.GetId(),
            .mesh_id = mesh.mesh_id,
            .material_id = material.material_id,
            .sort_key = ComputeSortKey(mesh.mesh_id, material.material_id),
        });
    }

    std::ranges::sort(result.opaque_commands, {}, &DrawCommand::sort_key);

    return result;
}
} // namespace se::graphics
