#pragma once
#include <memory_resource>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::core::memory::memory_resource
{
/**
 * 컨테이너에서 OsMemory를 사용할 수 있게 만든 Resource
 */
class [[deprecated]] SE_CORE_API OsMemoryResource : public std::pmr::memory_resource
{
public:
    OsMemoryResource() = default;

protected:
    virtual void* do_allocate(usize size, usize align) override;
    virtual void do_deallocate(void* ptr, usize size, usize align) override;
    [[nodiscard]] virtual bool do_is_equal(const memory_resource& other) const noexcept override;
};
}
