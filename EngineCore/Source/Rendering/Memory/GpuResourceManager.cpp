#include "Rendering/Memory/GpuResourceManager.h"

#include <cmath>
#include <cstring>


namespace se::rendering
{
GpuResourceManager::GpuResourceManager(SDL_GPUDevice* in_device)
    : device(in_device)
{
    SE_ASSERT(device, "GPU device is null!");
}

GpuResourceManager::~GpuResourceManager()
{
    // Texture 해제
    for (GpuTexture& gpu_texture : texture_map | std::views::values)
    {
        if (gpu_texture.IsValid())
        {
            SDL_ReleaseGPUTexture(device, gpu_texture.texture);
        }
    }
    texture_map.Clear();

    // Buffer 해제
    slice_map.Clear();
    geometry_blocks.Clear();
}

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
    if (!transfer_buffer)
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

void GpuResourceManager::UnloadMesh(const asset::AssetId& in_id)
{
    // TODO: Defragmentation 시스템 도입 시 실제 메모리 회수 로직 추가 필요
    slice_map.Remove(in_id);
}

const GpuBufferSlice& GpuResourceManager::GetSlice(const asset::AssetId& in_id) const
{
    return slice_map.Find(in_id).ValueOr(EmptySlice);
}

bool GpuResourceManager::UploadTexture(const asset::AssetId& in_id, const SDL_Surface* in_surface, TextureUploadSettings in_settings)
{
    // 포맷 변환 (SDL_Surface -> RGBA32)
    SDL_Surface* converted_surface = SDL_ConvertSurface(const_cast<SDL_Surface*>(in_surface), SDL_PIXELFORMAT_RGBA32);
    if (!converted_surface)
    {
        ConsoleLog(ELogLevel::Error, "Surface conversion failed: {}", SDL_GetError());
        return false;
    }

    const uint32 width = static_cast<uint32>(converted_surface->w);
    const uint32 height = static_cast<uint32>(converted_surface->h);
    const uint32 buffer_size = converted_surface->pitch * height;

    // Mipmap 레벨 계산
    uint32 num_levels = 1;
    if (in_settings.generate_mips)
    {
        // 1 + floor(log2(max(w, h)))
        num_levels = static_cast<uint32>(std::floor(std::log2(std::max(width, height)))) + 1;
    }

    // Texture 포맷 및 Usage 설정
    SDL_GPUTextureFormat format;
    if (in_settings.is_srgb)
    {
        format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    }
    else
    {
        format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    }

    SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    // 밉맵 생성을 하려면 GPU가 해당 텍스처에 렌더링(Blit)을 할 수 있어야 함
    if (in_settings.generate_mips)
    {
        usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    }

    // GPU Texture 생성
    const SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = format,
        .usage = usage,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = num_levels,
        .sample_count = SDL_GPU_SAMPLECOUNT_1
    };

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &create_info);
    if (!texture)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create GPU texture: {}", SDL_GetError());
        SDL_DestroySurface(converted_surface);
        return false;
    }

    // Transfer Buffer 생성
    const SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = buffer_size
    };

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer_buffer)
    {
        SDL_ReleaseGPUTexture(device, texture);
        SDL_DestroySurface(converted_surface);
        return false;
    }

    // 메모리 맵핑 및 복사 (CPU -> Transfer Buffer)
    if (void* mapped_ptr = SDL_MapGPUTransferBuffer(device, transfer_buffer, false))
    {
        std::memcpy(mapped_ptr, converted_surface->pixels, buffer_size);
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);
    }
    else
    {
        SDL_ReleaseGPUTexture(device, texture);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        SDL_DestroySurface(converted_surface);
        return false;
    }

    // GPU에 업로드
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);
    {
        const SDL_GPUTextureTransferInfo src_info = {
            .transfer_buffer = transfer_buffer,
            .offset = 0,
            .pixels_per_row = width,
            .rows_per_layer = height
        };

        const SDL_GPUTextureRegion dst_region = {
            .texture = texture,
            .mip_level = 0, // Base Level에만 업로드
            .w = width,
            .h = height,
            .d = 1
        };

        SDL_UploadToGPUTexture(copy_pass, &src_info, &dst_region, false);
    }
    SDL_EndGPUCopyPass(copy_pass);

    // 밉맵 생성 (업로드 직후 수행)
    if (in_settings.generate_mips && num_levels > 1)
    {
        SDL_GenerateMipmapsForGPUTexture(cmd, texture);
    }

    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    SDL_DestroySurface(converted_surface); // 변환된 임시 Surface 해제

    texture_map.Insert(in_id, {
        .texture = texture,
        .width = width,
        .height = height,
        .format = create_info.format
    });
    return true;
}

const GpuTexture& GpuResourceManager::GetTexture(const asset::AssetId& in_id) const
{
    return texture_map.Find(in_id).ValueOr(EmptyTexture);
}

void GpuResourceManager::UnloadTexture(const asset::AssetId& in_id)
{
    if (const Optional texture_opt = texture_map.Find(in_id))
    {
        if (texture_opt->IsValid())
        {
            SDL_ReleaseGPUTexture(device, texture_opt->texture);
        }
        texture_map.Remove(in_id);
    }
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
}  // namespace se::rendering
