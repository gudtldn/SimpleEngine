#include "Rendering/Memory/GpuResourceManager.h"


namespace se::rendering
{
GpuResourceManager::GpuResourceManager(SDL_GPUDevice* in_device)
    : device(in_device)
{
    SE_ASSERT(device, "GPU device is null!");
}

GpuResourceManager::~GpuResourceManager() = default;

bool GpuResourceManager::UploadMesh(
    const asset::AssetId& in_id,
    const void* in_vertex_data, uint32 in_vertex_size,
    const void* in_index_data, uint32 in_index_size
)
{
    // GPU 메모리 할당
    const uint32 total_size = in_vertex_size + in_index_size;
    if (total_size == 0)
    {
        return false;
    }

    GpuBufferSlice slice = AllocateInGeometryBlock(total_size);
    if (!slice.IsValid())
    {
        return false;
    }

    // Index 데이터는 Vertex 데이터 바로 뒤에 위치
    slice.index_offset = in_vertex_size;

    // Transfer Buffer 생성 (Staging)
    const SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = total_size
    };

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (transfer_buffer == nullptr)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create Transfer Buffer: {}", SDL_GetError());
        return false;
    }

    // 메모리 맵핑 및 복사 (CPU -> Transfer Buffer)
    if (void* mapped_ptr = SDL_MapGPUTransferBuffer(device, transfer_buffer, false))
    {
        uint8* cursor = static_cast<uint8*>(mapped_ptr);

        // Vertex 복사
        std::memcpy(cursor, in_vertex_data, in_vertex_size);

        // Index 복사
        if (in_index_data && in_index_size > 0)
        {
            std::memcpy(cursor + in_vertex_size, in_index_data, in_index_size);
        }

        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "Failed to map Transfer Buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    // GPU에 업로드
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTransferBufferLocation src_loc = {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        };

        const SDL_GPUBufferRegion dst_loc = {
            .buffer = slice.buffer,
            .offset = slice.offset,
            .size = total_size
        };

        SDL_UploadToGPUBuffer(copy_pass, &src_loc, &dst_loc, false);
    }
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    slice_map.Insert(in_id, slice);
    return true;
}

const GpuBufferSlice& GpuResourceManager::GetSlice(const asset::AssetId& in_id) const
{
    return slice_map.Find(in_id).ValueOr(EmptySlice);
}

GpuBufferSlice GpuResourceManager::AllocateInGeometryBlock(uint32 in_size)
{
    // Vector4를 고려해서 Vertex/Index Buffer는 16바이트 정렬로 설정
    constexpr uint32 alignment = 16;

    // 기존 블록에서 남은 공간 탐색
    for (GpuMemoryBlock& block : geometry_blocks)
    {
        if (GpuBufferSlice slice; block.AllocateSlice(in_size, alignment, slice))
        {
            return slice;
        }
    }

    // 공간이 없으면 새 블록 할당
    const uint32 new_block_size = std::max(in_size, DEFAULT_BLOCK_SIZE);

    // Geometry용 Usage: Vertex + Index (Unified)
    // 필요하다면 Storage Buffer Read 플래그도 추가 가능 (SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
    constexpr SDL_GPUBufferUsageFlags usage = SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_INDEX;
    geometry_blocks.Emplace(device, new_block_size, usage);
    GpuMemoryBlock& new_block = geometry_blocks.Back().Value();

    GpuBufferSlice slice;
    [[maybe_unused]] const bool result = new_block.AllocateSlice(in_size, alignment, slice);
    SE_ASSERT(result); // 방금 만든 블록이므로 반드시 성공해야 함

    return slice;
}
}
