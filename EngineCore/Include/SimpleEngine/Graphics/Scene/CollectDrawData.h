#pragma once

#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"

// forward declaration
namespace se { class World; }


namespace se::graphics
{
/** ECS World에서 렌더링 가능한 엔티티를 수집하여 SceneDrawData를 생성합니다. */
[[nodiscard]] SE_CORE_API SceneDrawData CollectDrawData(World& world);
} // namespace se::graphics
