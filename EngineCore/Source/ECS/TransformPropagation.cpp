#include "SimpleEngine/ECS/TransformPropagation.h"

#include "SimpleEngine/Core/Math/TransformUtility.h"
#include "SimpleEngine/ECS/Query.h"


namespace se
{
using namespace math;

void SyncGlobalTransforms(
    Commands commands,
    Query<Entity, With<TransformComponent>, Without<GlobalTransformComponent>> missing
)
{
    for (auto [entity] : missing)
    {
        commands.Entity(entity).Insert(GlobalTransformComponent{});
    }
}

namespace
{
void PropagateToChildren(
    const Matrix4x4& parent_world,
    bool parent_was_dirty,
    const ChildrenComponent& children,
    Query<TransformComponent&, GlobalTransformComponent&, Optional<const ChildrenComponent&>>& all
)
{
    for (const Entity& child : children.children)
    {
        const auto result = all.TryGet(child);
        if (!result)
        {
            continue;
        }

        auto& [local, global, children_opt] = *result;
        const bool needs_update = local.dirty || parent_was_dirty;

        if (needs_update)
        {
            global.value = TransformUtility::MakeModelMatrix(
                local.position,
                local.rotation,
                local.scale
            ) * parent_world;
            local.dirty = false;
        }

        if (children_opt)
        {
            PropagateToChildren(global.value, needs_update, *children_opt, all);
        }
    }
}
} // namespace

void PropagateTransforms(
    Query<Entity, TransformComponent&, GlobalTransformComponent&, Optional<const ChildrenComponent&>, Without<ParentComponent>> roots,
    Query<TransformComponent&, GlobalTransformComponent&, Optional<const ChildrenComponent&>> all
)
{
    for (auto [entity, local, global, children_opt] : roots)
    {
        const bool was_dirty = local.dirty;

        if (was_dirty)
        {
            global.value = TransformUtility::MakeModelMatrix(local.position, local.rotation, local.scale);
            local.dirty = false;
        }

        if (children_opt)
        {
            PropagateToChildren(global.value, was_dirty, *children_opt, all);
        }
    }
}
} // namespace se
