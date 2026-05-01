#include "SimpleEngine/Graphics/Scene/CollectDrawData.h"

#include "SimpleEngine/ECS/Query.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"

#include <algorithm>


namespace se
{

namespace
{
/** DrawCommand의 정렬 키를 계산합니다. */
[[nodiscard]] uint64 ComputeSortKey(const AssetId& mesh_id)
{
    const uint64 mesh_hash = std::hash<AssetId>{}(mesh_id);
    return mesh_hash;
}
} // namespace


SceneDrawData CollectDrawData(const World& world)
{
    SceneDrawData result;

    const auto query = world.CreateQuery<Entity, const GlobalTransformComponent&, const StaticMeshComponent&>();
    for (const auto [entity, global_transform, mesh] : query)
    {
        result.opaque_commands.Push({
            .model_matrix = global_transform.value,
            .entity_id = entity.GetId(),
            .mesh_id = mesh.mesh_id,
            .sort_key = ComputeSortKey(mesh.mesh_id),
        });
    }

    std::ranges::sort(result.opaque_commands, {}, &DrawCommand::sort_key);

    return result;
}
} // namespace se
