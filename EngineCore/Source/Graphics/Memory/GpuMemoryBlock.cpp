#include "SimpleEngine/Graphics/Memory/GpuMemoryBlock.h"

#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/Common.h"


namespace se::graphics
{
GpuMemoryBlock::GpuMemoryBlock(RenderDevice* in_render_device, uint32 in_size, SDL_GPUBufferUsageFlags in_usage)
    : render_device(in_render_device)
    , usage_flags(in_usage)
    , total_size(in_size)
{
    const SDL_GPUBufferCreateInfo buffer_info = {
        .usage = in_usage,
        .size = in_size,
    };
    buffer = SDL_CreateGPUBuffer(render_device->GetRawDevice(), &buffer_info);
}

GpuMemoryBlock::~GpuMemoryBlock()
{
    if (buffer && render_device)
    {
        SDL_ReleaseGPUBuffer(render_device->GetRawDevice(), buffer);
    }
}

GpuMemoryBlock::GpuMemoryBlock(GpuMemoryBlock&& other) noexcept
{
    *this = std::move(other);
}

GpuMemoryBlock& GpuMemoryBlock::operator=(GpuMemoryBlock&& other) noexcept
{
    if (this != &other)
    {
         if (buffer)
        {
            SDL_ReleaseGPUBuffer(render_device->GetRawDevice(), buffer);
        }

        static_assert(
            AlignedSize<alignof(GpuMemoryBlock)>(
                sizeof(render_device) // NOLINT(*-sizeof-expression)
                + sizeof(buffer)      // NOLINT(*-sizeof-expression)
                + sizeof(usage_flags)
                + sizeof(total_size)
                + sizeof(used_offset)
            ) == sizeof(GpuMemoryBlock),
            "GpuMemoryBlock size mismatch"
        );

        render_device = std::exchange(other.render_device, nullptr);
        buffer = std::exchange(other.buffer, nullptr);
        usage_flags = std::exchange(other.usage_flags, 0);
        total_size = std::exchange(other.total_size, 0);
        used_offset = std::exchange(other.used_offset, 0);
    }
    return *this;
}

bool GpuMemoryBlock::AllocateSlice(uint32 in_size, uint32 in_alignment, GpuBufferSlice& out_slice)
{
    const uint32 aligned_offset = static_cast<uint32>(AlignedSize(used_offset, in_alignment));

    // Buffer의 남은 공간이 부족한지?
    if (aligned_offset + in_size > total_size)
    {
        return false; // 공간 부족
    }

    out_slice.buffer = buffer;
    out_slice.offset = aligned_offset;
    out_slice.size = in_size;

    // index_offset는 여기서 알 수 없으므로, GpuResourceManager가 설정
    out_slice.index_offset = 0;
    out_slice.index_count = 0;

    // 다음 할당을 위해 Offset 이동
    used_offset = aligned_offset + in_size;
    return true;
}
}  // namespace se::graphics
