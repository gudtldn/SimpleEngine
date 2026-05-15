#include "SimpleEngine/Graphics/Memory/GpuMemoryBlock.h"

#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/Common.h"


namespace se
{
GpuMemoryBlock::GpuMemoryBlock(RenderDevice* in_render_device, u32 in_size, SDL_GPUBufferUsageFlags in_usage)
    : render_device(in_render_device)
    , usage_flags(in_usage)
    , total_size(in_size)
{
    const SDL_GPUBufferCreateInfo buffer_info = {
        .usage = in_usage,
        .size = in_size,
    };
    buffer_rid = render_device->CreateBuffer(buffer_info, "GpuMemoryBlock");
}

GpuMemoryBlock::~GpuMemoryBlock()
{
    if (render_device && buffer_rid.IsValid())
    {
        render_device->DestroyBuffer(buffer_rid);
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
        if (render_device && buffer_rid.IsValid())
        {
            render_device->DestroyBuffer(buffer_rid);
        }

        static_assert(
            AlignedSize<alignof(GpuMemoryBlock)>(
                sizeof(render_device) // NOLINT(*-sizeof-expression)
                + sizeof(buffer_rid)
                + sizeof(usage_flags)
                + sizeof(total_size)
                + sizeof(used_offset)
            ) == sizeof(GpuMemoryBlock),
            "GpuMemoryBlock size mismatch"
        );

        render_device = std::exchange(other.render_device, nullptr);
        buffer_rid = std::exchange(other.buffer_rid, RID{});
        usage_flags = std::exchange(other.usage_flags, 0);
        total_size = std::exchange(other.total_size, 0);
        used_offset = std::exchange(other.used_offset, 0);
    }
    return *this;
}

SDL_GPUBuffer* GpuMemoryBlock::GetNativeBuffer() const
{
    if (!render_device || !buffer_rid.IsValid())
    {
        return nullptr;
    }
    if (const auto resource = render_device->GetBuffer(buffer_rid))
    {
        return resource->handle;
    }
    return nullptr;
}

bool GpuMemoryBlock::AllocateSlice(u32 in_size, u32 in_alignment, GpuBufferSlice& out_slice)
{
    const u32 aligned_offset = static_cast<u32>(AlignedSize(used_offset, in_alignment));

    // Buffer의 남은 공간이 부족한지?
    if (aligned_offset + in_size > total_size)
    {
        return false; // 공간 부족
    }

    out_slice.buffer = GetNativeBuffer();
    out_slice.offset = aligned_offset;
    out_slice.size = in_size;

    // index_offset는 여기서 알 수 없으므로, GpuResourceManager가 설정
    out_slice.index_offset = 0;
    out_slice.index_count = 0;

    // 다음 할당을 위해 Offset 이동
    used_offset = aligned_offset + in_size;
    return true;
}
} // namespace se
