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

    // 서브 메시(MeshSection) 파라미터. section_index_count가 0이면 GpuBufferSlice 전체를 그립니다.
    uint32 section_first_index = 0;  // Section 시작 인덱스 (SDL first_index)
    uint32 section_index_count = 0;  // Section 인덱스 수
    int32 section_vertex_offset = 0; // SDL base vertex offset
};
} // namespace se::graphics
