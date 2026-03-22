#pragma once

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResources.h"
#include "SimpleEngine/Graphics/RenderPass/RenderPassBase.h"

#include <limits>
#include <memory>


namespace se::graphics
{
static constexpr uint32 INVALID_PASS_INDEX = std::numeric_limits<uint32>::max();

/** 그래프 내의 렌더 패스를 표현하는 내부 구조체 */
struct RGPassNode
{
    StringName name;
    std::unique_ptr<RenderPassBase> pass_object;
    HashSet<uint32> read_indices;  // resource_nodes 배열 내 인덱스
    HashSet<uint32> write_indices; // resource_nodes 배열 내 인덱스

    // Compile() 단계에서 채워질 정보들
    bool culled = true; // 이번 프레임에서 사용 안하는지 여부 (Compile때 false로 변경)
};

/** 그래프 내의 리소스를 표현하는 내부 구조체 */
struct RGResourceNode
{
    StringName name;
    std::unique_ptr<RGResourceBase> resource;

    // 이 리소스를 쓰는 패스 인덱스
    uint32 writer_pass_index = INVALID_PASS_INDEX;

    // lifetime 정보 (위상 정렬 후 계산됨)
    uint32 first_user_pass_index = std::numeric_limits<uint32>::max();
    uint32 last_user_pass_index = 0;
};
} // namespace se::graphics