#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/ECS/Entity.h"

#include <limits>


namespace se::graphics
{
/**
 * 개별 오브젝트의 Draw Command를 나타내는 구조체
 */
struct DrawCommand
{
    Matrix4x4 model_matrix = Matrix4x4::Identity();
    uint32 entity_id = Entity::Invalid;
    asset::AssetId mesh_id = asset::AssetId::Invalid;
    asset::AssetId material_id = asset::AssetId::Invalid;
    uint64 sort_key = 0;

    // FrameMaterialCache의 슬롯 인덱스. uint16::max()면 유효한 material이 없는 경우.
    uint16 material_slot_index = std::numeric_limits<uint16>::max();
};
} // namespace se::graphics
