#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/ECS/Entity.h"

#include <limits>


namespace se
{
/**
* GPU Multi-Draw Indirect(MDI)의 정보를 담은 구조체
 */
struct IndirectDrawCommand
{
    /** 그릴 인덱스 총 개수 (MeshSection::index_count) */
    uint32 index_count;

    /** 인스턴싱 개수 (현재 1로 고정, 향후 하드웨어 인스턴싱 지원 시 활용) */
    uint32 instance_count;

    /** 인덱스 버퍼 내에서 이 섹션이 시작되는 오프셋 (MeshSection::index_offset) */
    uint32 first_index;

    /**
     * 통합 버텍스 버퍼를 사용할 경우의 베이스 버텍스 오프셋 (MeshSection::vertex_offset)
     * @note Vulkan/DX 스펙상 음수 시프팅이 가능해야 하므로 반드시 int32 타입이어야 합니다.
     */
    int32 vertex_offset;

    /**
     * 셰이더 내부에서 현재 그리고 있는 오브젝트/인스턴스 데이터를 찾기 위한 인덱스
     * @warning 크로스플랫폼 셰이더 호환성 문제로 인해 이 값은 항상 0으로 넘기고,
     *          실제 인스턴스 ID는 Push Constant나 별도의 UBO로 셰이더에 넘기는 것이 안전합니다.
     *          (Vulkan(gl_InstanceIndex)과 DX12(SV_InstanceID)는 셰이더 내장 변수 동작이 다름)
     */
    uint32 first_instance;
};
static_assert(sizeof(IndirectDrawCommand) == 20, "MDI struct MUST be exactly 20 bytes for GPU compatibility.");

/**
 * CPU에서 렌더링 파이프라인을 준비하고 정렬(Sorting)하기 위해 관리하는 논리적 드로우 커맨드.
 * Entity 단위가 아닌 '서브 메시(MeshSection) 단위'로 생성됩니다.
 */
struct DrawCommand
{
    /**
     * Model의 World Transform Matrix
     * @todo 나중에 인스턴스 개수가 많아지면 DrawCommand가 아닌, SSBO 배열로 전달해야 함.
     */
    Matrix4x4 model_matrix = Matrix4x4::Identity();

    /** 식별용 Entity ID (에디터 Picking용) */
    uint32 entity_id = Entity::Invalid;

    // TODO: ForwardScenePass 리팩토링 후 제거 예정. 현재 코드 호환성을 위해 당분간 유지
    [[deprecated]] AssetId mesh_id;

    /** 렌더링시 비슷한 유형끼리 정렬을 위한 Key */
    uint64 sort_key = 0;

    /**
     * SceneDrawData의 FrameMaterialCache내 slots 배열 인덱스
     * @note 런타임에 렌더 스레드는 이 인덱스를 사용해 해당 섹션에 바인딩할 머티리얼 파라미터(UBO)와 텍스처를 조회합니다.
     */
    uint16 material_slot_index = std::numeric_limits<uint16>::max();

    /** MDI 커맨드 파라미터 */
    IndirectDrawCommand draw_params = {};
};
} // namespace se
