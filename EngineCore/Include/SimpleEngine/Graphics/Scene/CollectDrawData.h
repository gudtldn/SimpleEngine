#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se
{
// forward declaration
class World;
class AssetSubsystem;
class GpuResourceManager;

/** ECS World에서 렌더링 가능한 엔티티를 수집하여 SceneDrawData를 생성합니다. */
[[nodiscard]] SE_CORE_API SceneDrawData CollectDrawData(
    const World& world,
    ArrayView<const RenderView> views,
    const AssetSubsystem& asset_subsystem,
    const GpuResourceManager& gpu_manager
);
} // namespace se
