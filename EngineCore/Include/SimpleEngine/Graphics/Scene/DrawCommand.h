#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"


namespace se::graphics
{
/**
 * 개별 오브젝트의 Draw Command를 나타내는 구조체
 */
struct DrawCommand
{
    Matrix4x4 model_matrix;
    asset::AssetId mesh_id;
    asset::AssetId material_id;
    uint64 sort_key = 0;
};
} // namespace se::graphics
