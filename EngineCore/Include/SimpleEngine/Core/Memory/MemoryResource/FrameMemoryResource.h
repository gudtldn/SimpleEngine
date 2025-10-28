#pragma once
#include <memory_resource>
#include <span>
#include <type_traits>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::core::memory::memory_resource
{
/**
 * 프레임과 같이 수명이 짧고 한꺼번에 해제되는 메모리에 특화된 리소스
 * @tparam BufferSize
 */
template <size_t BufferSize = std::dynamic_extent>
class [[deprecated]] FrameMemoryResource : public std::pmr::memory_resource
{
public:
    /**
     * 동적 할당용 생성자
     * @param upstream 이 리소스가 실제 메모리를 할당/해제할 때 사용할 상위 리소스
     */
    explicit FrameMemoryResource(
        memory_resource* upstream = std::pmr::get_default_resource()
    ) requires (BufferSize != std::dynamic_extent);

    /**
     * 스택용 생성자
     * @param buffer_size 내부적으로 할당할 버퍼의 크기 (Byte)
     * @param upstream 이 리소스가 실제 메모리를 할당/해제할 때 사용할 상위 리소스
     */
    explicit FrameMemoryResource(
        size_t buffer_size,
        memory_resource* upstream = std::pmr::get_default_resource()
    ) requires (BufferSize == std::dynamic_extent);

protected:
    virtual void* do_allocate(size_t size, size_t align) override;
    virtual void do_deallocate(void* ptr, size_t size, size_t align) override;
    virtual bool do_is_equal(const memory_resource& other) const noexcept override;

private:
    using BufferType = std::conditional_t<
        BufferSize == std::dynamic_extent,
        std::vector<uint8>,
        std::array<uint8, BufferSize>
    >;

    BufferType buffer;
    std::pmr::monotonic_buffer_resource monotonic_resource;
};


template <size_t BufferSize>
FrameMemoryResource<BufferSize>::FrameMemoryResource(memory_resource* upstream) requires (BufferSize != std::dynamic_extent)
    : monotonic_resource(buffer.data(), buffer.size(), upstream)
{
}

template <size_t BufferSize>
FrameMemoryResource<BufferSize>::FrameMemoryResource(size_t buffer_size, memory_resource* upstream) requires (BufferSize == std::dynamic_extent)
    : buffer(buffer_size, upstream)
    , monotonic_resource(buffer.data(), buffer.size(), upstream)
{
}

template <size_t BufferSize>
void* FrameMemoryResource<BufferSize>::do_allocate(size_t size, size_t align)
{
    return monotonic_resource.allocate(size, align);
}

template <size_t BufferSize>
void FrameMemoryResource<BufferSize>::do_deallocate(void* ptr, size_t size, size_t align)
{
    monotonic_resource.deallocate(ptr, size, align);
}

template <size_t BufferSize>
bool FrameMemoryResource<BufferSize>::do_is_equal(const memory_resource& other) const noexcept
{
    return this == &other;
}
}
