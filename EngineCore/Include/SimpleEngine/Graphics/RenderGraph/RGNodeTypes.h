#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"

#include <limits>
#include <memory>


namespace se::graphics
{
static constexpr uint32 INVALID_PASS_INDEX = std::numeric_limits<uint32>::max();

/**
 * 패스가 특정 버전의 리소스를 참조할 때 사용하는 구조체
 * resource_index + version 쌍으로 리소스의 논리적 상태를 식별합니다.
 */
struct RGResourceRef
{
    uint32 resource_index;
    uint32 version;
};

/** 그래프 내의 렌더 패스를 표현하는 내부 구조체 */
struct RGPassNode
{
    StringName name;
    std::unique_ptr<RenderPassBase> pass_object;

    // --- Setup() 단계에서 수집 (버전 없음) ---

    Array<uint32> write_indices; // 이 패스가 쓰는 리소스 인덱스
    Array<uint32> read_indices;  // 이 패스가 읽는 리소스 인덱스

    // --- Compile() 단계에서 자동으로 채워짐 ---

    /** 이 패스가 읽는 리소스와 버전 목록 */
    Array<RGResourceRef> read_refs;

    /**
     * 이 패스가 쓰는 리소스와 출력 버전의 매핑
     * Key: resource_nodes 배열 내 인덱스, Value: 이 패스가 생성하는 출력 버전
     */
    HashMap<uint32, uint32> write_map;

    bool culled = true; // 이번 프레임에서 사용 안하는지 여부 (Compile때 false로 변경)
};

/** 그래프 내의 리소스를 표현하는 내부 구조체 */
struct RGResourceNode
{
    StringName name;
    std::unique_ptr<RGResourceBase> resource;

    /**
     * 이 리소스의 각 출력 버전을 생성하는 패스 인덱스의 매핑
     * Key: 출력 버전, Value: 해당 버전을 생성하는 pass_nodes 배열 내 인덱스
     */
    HashMap<uint32, uint32> version_to_writer;

    // lifetime 정보 (위상 정렬 후 계산됨)
    uint32 first_user_pass_index = std::numeric_limits<uint32>::max();
    uint32 last_user_pass_index = 0;
};
} // namespace se::graphics
