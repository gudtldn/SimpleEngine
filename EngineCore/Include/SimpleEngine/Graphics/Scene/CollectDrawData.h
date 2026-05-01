#pragma once

#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"


namespace se
{
// forward declaration
class World;
class AssetSubsystem;

/** ECS World에서 렌더링 가능한 엔티티를 수집하여 SceneDrawData를 생성합니다. */
[[nodiscard]] SE_CORE_API SceneDrawData CollectDrawData(const World& world, const AssetSubsystem& asset_subsystem);
} // namespace se
