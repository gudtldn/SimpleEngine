#pragma once

#include "SimpleEngine/ECS/Components/ChildrenComponent.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/ParentComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"
#include "SimpleEngine/ECS/Commands.h"


namespace se
{
/**
 * TransformComponent를 가진 Entity에 GlobalTransformComponent가 없으면 자동 추가합니다.
 * PropagateTransforms보다 먼저 실행되어야 합니다.
 */
SE_CORE_API void SyncGlobalTransforms(
    Commands commands,
    Query<Entity, With<TransformComponent>, Without<GlobalTransformComponent>> missing
);

/**
 * 계층 구조를 따라 GlobalTransformComponent를 전파합니다.
 * 루트 Entity(ParentComponent 없음)부터 시작하여 자식으로 재귀 전파합니다.
 */
SE_CORE_API void PropagateTransforms(
    Query<Entity, TransformComponent&, GlobalTransformComponent&, Optional<const ChildrenComponent&>, Without<ParentComponent>> roots,
    Query<TransformComponent&, GlobalTransformComponent&, Optional<const ChildrenComponent&>> all
);
} // namespace se
