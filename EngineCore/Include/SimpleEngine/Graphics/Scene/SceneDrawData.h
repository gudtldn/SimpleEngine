#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"
#include "SimpleEngine/Graphics/Scene/DrawCommand.h"


namespace se
{
/**
 * DrawCall에 필요한 텍스처 바인딩 정보
 */
struct TextureBinding
{
    /** 셰이더 내에서 텍스처가 바인딩될 프래그먼트 슬롯 번호 */
    uint32 fragment_slot = 0;

    /** 바인딩할 텍스처 에셋의 ID */
    AssetId texture_id = AssetId::Invalid;

    /** 텍스처 샘플링에 사용할 샘플러 타입 */
    ESamplerType sampler = ESamplerType::LinearRepeat;
};

/**
 * 한 프레임 동안 사용될 머티리얼 데이터(UBO 바이트 배열 및 텍스처 바인딩 정보)를
 * 렌더 스레드에 캐시 효율적으로 넘겨주기 위한 메모리 아레나(Arena) 구조체
 */
struct FrameMaterialCache
{
    /** 단일 머티리얼 인스턴스가 사용하는 데이터의 오프셋과 크기 정보 */
    struct MaterialSlot
    {
        uint32 ubo_offset = 0; // ubo_arena 배열 내 시작 위치 (바이트 단위)
        uint16 ubo_size = 0;   // 셰이더에 전달할 UBO 데이터 크기 (바이트 단위)

        uint32 binding_offset = 0; // binding_arena 배열 내 시작 인덱스
        uint16 binding_count = 0;  // 바인딩할 텍스처 총 개수
    };

    /** Scene에 존재하는 모든 Renderable의 머티리얼 상수(Constant) 바이트를 일렬로 팩킹한 배열 */
    Array<uint8> ubo_arena;

    /** 모든 텍스처 바인딩 정보를 일렬로 팩킹한 배열 */
    Array<TextureBinding> binding_arena;

    /**
     * 슬롯 정보들의 배열
     * DrawCommand의 `material_slot_index`가 이 배열의 특정 인덱스를 가리키게 됩니다.
     */
    Array<MaterialSlot> slots;

    /**
     * 동일한 Material Asset이 여러 엔티티에서 사용될 경우, 아레나 내 중복 저장을 방지하기 위한 캐시 매핑
     * Material AssetId를 키로 사용하여 기존에 할당된 슬롯 인덱스(slots의 인덱스)를 빠르게 반환합니다.
     */
    HashMap<AssetId, uint16> material_to_slot;
};

/**
 * 한 프레임에서 렌더링할 드로우 커맨드의 스냅샷
 */
struct SceneDrawData
{
    /** 불투명 객체들을 위한 드로우 커맨드 목록 */
    Array<DrawCommand> opaque_commands;

    // Array<DrawCommand> transparent_commands; // 추후 투명 오브젝트 분류 시 추가

    /** 렌더 스레드가 참조할 머티리얼 파라미터 및 텍스처 바인딩 데이터 아레나 */
    FrameMaterialCache material_cache;

    // -------------------------------------------------------------------------
    // [데이터 흐름 & 스레드 안전성 보장 (Data Flow & Thread Safety)]
    // -------------------------------------------------------------------------

    /** 아직 CPU에 로드되지 않은 메시 ID 목록 (지연 로딩 트리거용) */
    Array<AssetId> requested_meshes;

    /** 아직 CPU에 로드되지 않은 머티리얼 인스턴스 ID 목록 (지연 로딩 트리거용) */
    Array<AssetId> requested_material_instances;

    /** 이번 프레임에 렌더링되는 모든 StaticMesh 에셋들의 수명 유지 핸들 배열 */
    HashMap<AssetId, AssetHandle<StaticMesh>> pinned_meshes;

    /** 이번 프레임에 렌더링되는 Material 에셋들의 수명 유지 핸들 배열 */
    HashMap<AssetId, AssetHandle<Material>> pinned_materials;

    /** 이번 프레임에 렌더링되는 MaterialInstance 에셋들의 수명 유지 핸들 배열 */
    HashMap<AssetId, AssetHandle<MaterialInstance>> pinned_material_instances;
};
} // namespace se
