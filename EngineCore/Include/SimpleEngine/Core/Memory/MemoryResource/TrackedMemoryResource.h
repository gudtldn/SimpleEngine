#pragma once
#include <memory_resource>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::core::memory::memory_resource
{
/**
 * 컨테이너의 사용량을 추적하는 메모리 리소스
 */
class [[deprecated]] SE_CORE_API TrackedMemoryResource : public std::pmr::memory_resource
{
public:
    /**
     * @param upstream 이 리소스가 실제 메모리를 할당/해제할 때 사용할 상위 리소스.
     *                 기본값은 std::pmr::new_delete_resource()로, 전역 new/delete를 사용합니다.
     */
    explicit TrackedMemoryResource(memory_resource* upstream = std::pmr::get_default_resource());

protected:
    virtual void* do_allocate(usize size, usize align) override;
    virtual void do_deallocate(void* ptr, usize size, usize align) override;
    [[nodiscard]] virtual bool do_is_equal(const memory_resource& other) const noexcept override;

private:
    memory_resource* upstream_resource;
};
}
