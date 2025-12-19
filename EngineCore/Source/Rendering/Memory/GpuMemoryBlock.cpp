#include "Rendering/Memory/GpuMemoryBlock.h"
#include "Utility/Common.h"


namespace se::rendering
{
GpuMemoryBlock::GpuMemoryBlock(SDL_GPUDevice* in_device, uint32 in_size, SDL_GPUBufferUsageFlags in_usage)
    : device(in_device)
    , usage_flags(in_usage)
    , total_size(in_size)
{
    const SDL_GPUBufferCreateInfo buffer_info = {
        .usage = in_usage,
        .size = in_size,
    };
    buffer = SDL_CreateGPUBuffer(device, &buffer_info);
    // SDL_SetGPUBufferName(device, buffer, 이름 설정);
}

GpuMemoryBlock::~GpuMemoryBlock()
{
    if (buffer)
    {
        SDL_ReleaseGPUBuffer(device, buffer);
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
            SDL_ReleaseGPUBuffer(device, buffer);
        }

        static_assert(
            utility::AlignedSize<alignof(GpuMemoryBlock)>(
                sizeof(device)   // NOLINT(*-sizeof-expression)
                + sizeof(buffer) // NOLINT(*-sizeof-expression)
                + sizeof(usage_flags)
                + sizeof(total_size)
                + sizeof(used_offset)
            ) == sizeof(GpuMemoryBlock),
            "GpuMemoryBlock size mismatch"
        );

        device = std::exchange(other.device, nullptr);
        buffer = std::exchange(other.buffer, nullptr);
        usage_flags = std::exchange(other.usage_flags, 0);
        total_size = std::exchange(other.total_size, 0);
        used_offset = std::exchange(other.used_offset, 0);
    }
    return *this;
}

bool GpuMemoryBlock::AllocateSlice(uint32 in_size, uint32 in_alignment, GpuBufferSlice& out_slice)
{
    const uint32 aligned_offset = static_cast<uint32>(utility::AlignedSize(used_offset, in_alignment));

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

    // 다음 할당을 위해 Offset 이동
    used_offset = aligned_offset + in_size;
    return true;
}
}  // namespace se::rendering
