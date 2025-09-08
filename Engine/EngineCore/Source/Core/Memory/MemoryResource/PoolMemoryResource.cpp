module SE.Core;
import :Memory.MemoryResource.PoolMemoryResource;


namespace se::core::memory::memory_resource
{
PoolMemoryResource::PoolMemoryResource(memory_resource* upstream)
    : synchronized_pool_resource(upstream)
{
}

PoolMemoryResource::PoolMemoryResource(const std::pmr::pool_options& options, memory_resource* upstream)
    : synchronized_pool_resource(options, upstream)
{
}
}
