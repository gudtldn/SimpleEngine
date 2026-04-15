#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/ECS/Entity.h"


namespace se::graphics
{
/**
 * 개별 오브젝트의 Draw Command를 나타내는 구조체
 */
struct DrawCommand
{
    Matrix4x4 model_matrix;
    uint32 entity_id = Entity::Invalid;
    asset::AssetId mesh_id;
    asset::AssetId material_id;
    uint64 sort_key = 0;
};
} // namespace se::graphics
