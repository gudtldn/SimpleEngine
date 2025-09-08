export module SE.Core:Memory.MemoryResource.OsMemoryResource;

import SE.Types;
import std;


export namespace se::core::memory::memory_resource
{
/**
 * 컨테이너에서 OsMemory를 사용할 수 있게 만든 Resource
 */
class OsMemoryResource : public std::pmr::memory_resource
{
public:
    OsMemoryResource() = default;

protected:
    virtual void* do_allocate(size_t size, size_t align) override;
    virtual void do_deallocate(void* ptr, size_t size, size_t align) override;
    virtual bool do_is_equal(const memory_resource& other) const noexcept override;
};
}
