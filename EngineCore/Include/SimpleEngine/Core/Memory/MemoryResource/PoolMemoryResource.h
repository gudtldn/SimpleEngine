#pragma once
#include <memory_resource>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::core::memory::memory_resource
{
class [[deprecated]] SE_CORE_API PoolMemoryResource : public std::pmr::synchronized_pool_resource
{
public:
    explicit PoolMemoryResource(memory_resource* upstream = std::pmr::get_default_resource());

    /**
     * @param options 풀의 동작을 제어하는 `std::pmr::pool_options` 구조체입니다.
     *        - `max_blocks_per_chunk`: 하나의 메모리 청크에 포함될 최대 블록 수를 지정합니다.
     *                                  값이 클수록 상위 리소스 호출은 줄지만 메모리 사용량이 많아질 수 있습니다.
     *        - `largest_required_pool_block`: 이 풀이 직접 처리할 가장 큰 메모리 블록의 크기를 지정합니다.
     *                                         이보다 큰 할당 요청은 상위 리소스로 바로 전달됩니다.
     * @param upstream 이 리소스가 실제 메모리를 할당/해제할 때 사용할 상위 리소스
     */
    explicit PoolMemoryResource(
        const std::pmr::pool_options& options,
        memory_resource* upstream = std::pmr::get_default_resource()
    );
};
}
