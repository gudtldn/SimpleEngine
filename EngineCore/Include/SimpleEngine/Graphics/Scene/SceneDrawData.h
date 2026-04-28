#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"
#include "SimpleEngine/Graphics/Scene/DrawCommand.h"


namespace se::graphics
{
/**
 * 씬의 텍스처 바인딩 정보
 */
struct TextureBinding
{
    uint32 fragment_slot = 0;
    asset::AssetId texture_id;
    ESamplerType sampler = ESamplerType::LinearRepeat;
};


/**
 * 프레임 단위 Material 데이터 캐시
 * 동일 material_id를 가진 DrawCommand들이 UBO/binding 데이터를 공유하여
 * per-draw heap alloc을 제거하고 렌더 스레드가 AssetSubsystem 없이 데이터를 읽을 수 있게 합니다.
 */
struct FrameMaterialCache
{
    struct MaterialSlot
    {
        uint32 ubo_offset = 0;
        uint16 ubo_size = 0;
        uint32 binding_offset = 0;
        uint16 binding_count = 0;
    };

    Array<uint8> ubo_arena;
    Array<TextureBinding> binding_arena;
    Array<MaterialSlot> slots;
    HashMap<asset::AssetId, uint16> material_to_slot;

    void Clear()
    {
        ubo_arena.Clear();
        binding_arena.Clear();
        slots.Clear();
        material_to_slot.Clear();
    }
};

/**
 * 한 프레임에서 렌더링할 드로우 커맨드의 스냅샷
 */
struct SceneDrawData
{
    Array<DrawCommand> opaque_commands;
    // Array<DrawCommand> transparent_commands; // 추후 투명 오브젝트 분류 시 추가

    FrameMaterialCache material_cache;
};
} // namespace se::graphics
