#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"


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

    // 게임 스레드에서 복사한 Material Instance Data
    Array<uint8> material_ubo_bytes; // fragment UBO raw bytes

    struct TextureBinding
    {
        uint32 fragment_slot = 0;
        asset::AssetId texture_id;
        ESamplerType sampler = ESamplerType::LinearRepeat;
    };
    Array<TextureBinding> texture_bindings;
};
} // namespace se::graphics
