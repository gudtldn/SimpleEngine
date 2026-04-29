#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>


namespace se::graphics
{
namespace
{
SDL_GPUTextureFormat ToSDLFormat(asset::ETextureFormat fmt)
{
    switch (fmt)
    {
    case asset::ETextureFormat::R8_UNORM:            return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    case asset::ETextureFormat::R8G8_UNORM:          return SDL_GPU_TEXTUREFORMAT_R8G8_UNORM;
    case asset::ETextureFormat::R8G8B8A8_UNORM:      return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    case asset::ETextureFormat::R8G8B8A8_UNORM_SRGB: return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    case asset::ETextureFormat::R16G16_FLOAT:        return SDL_GPU_TEXTUREFORMAT_R16G16_FLOAT;
    case asset::ETextureFormat::R16G16B16A16_FLOAT:  return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    case asset::ETextureFormat::R11G11B10_UFLOAT:    return SDL_GPU_TEXTUREFORMAT_R11G11B10_UFLOAT;
    case asset::ETextureFormat::BC1_UNORM:           return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM;
    case asset::ETextureFormat::BC1_UNORM_SRGB:      return SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM_SRGB;
    case asset::ETextureFormat::BC3_UNORM:           return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM;
    case asset::ETextureFormat::BC3_UNORM_SRGB:      return SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM_SRGB;
    case asset::ETextureFormat::BC4_UNORM:           return SDL_GPU_TEXTUREFORMAT_BC4_R_UNORM;
    case asset::ETextureFormat::BC5_UNORM:           return SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM;
    case asset::ETextureFormat::BC7_UNORM:           return SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM;
    case asset::ETextureFormat::BC7_UNORM_SRGB:      return SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM_SRGB;
    default:                                         return SDL_GPU_TEXTUREFORMAT_INVALID;
    }
}
} // namespace

GpuResourceManager::GpuResourceManager(RenderDevice& in_render_device)
    : render_device(&in_render_device)
{
}

GpuResourceManager::~GpuResourceManager()
{
    // Texture 해제
    for (const RID gpu_texture_id : texture_map | std::views::values)
    {
        render_device->DestroyTexture(gpu_texture_id);
    }
    texture_map.Clear();

    // Deferred Queue에 넣은 Texture를 바로 해제
    render_device->ProcessDeferredDestructions();

    // Buffer 해제
    slice_map.Clear();
    geometry_blocks.Clear();
}

bool GpuResourceManager::UploadMesh(
    SDL_GPUCommandBuffer* in_cmd,
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
    if (slice.buffer == nullptr)
    {
        return false;
    }

    // Index 데이터는 Vertex 데이터 바로 뒤에 위치
    slice.index_offset = slice.offset + in_vertex_size;
    slice.index_count = in_index_size / sizeof(uint32);

    // Transfer Buffer 생성 (Staging)
    const SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = total_size
    };

    // TODO: transfer_buffer를 매번 할당하는 대신, 추후 Ring Buffer 방식으로 개선
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &transfer_info);
    if (!transfer_buffer)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create Transfer Buffer: {}", SDL_GetError());
        return false;
    }

    // 메모리 맵핑 및 복사 (CPU -> Transfer Buffer)
    if (void* mapped_ptr = SDL_MapGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer, false))
    {
        uint8* cursor = static_cast<uint8*>(mapped_ptr);

        // Vertex 복사
        std::memcpy(cursor, in_vertex_data, in_vertex_size);

        // Index 복사
        if (in_index_data && in_index_size > 0)
        {
            std::memcpy(cursor + in_vertex_size, in_index_data, in_index_size);
        }

        SDL_UnmapGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "Failed to map Transfer Buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);
        return false;
    }

    // GPU에 업로드
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(in_cmd);
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
    SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);

    slice_map.Insert(in_id, slice);
    return true;
}

void GpuResourceManager::UnloadMesh(const asset::AssetId& in_id)
{
    // TODO: Defragmentation 시스템 도입 시 실제 메모리 회수 로직 추가 필요
    slice_map.Remove(in_id);
    mesh_residency_keys.Remove(in_id);
}

Optional<const GpuBufferSlice&> GpuResourceManager::GetSlice(const asset::AssetId& in_id) const
{
    return slice_map.Find(in_id);
}

Optional<const GpuResourceManager::MeshResidencyKey&> GpuResourceManager::GetMeshResidencyKey(const asset::AssetId& in_id) const
{
    return mesh_residency_keys.Find(in_id);
}

void GpuResourceManager::SetMeshResidencyKey(const asset::AssetId& in_id, MeshResidencyKey in_key)
{
    mesh_residency_keys.Insert(in_id, std::move(in_key));
}

void GpuResourceManager::RemoveMeshResidencyKey(const asset::AssetId& in_id)
{
    mesh_residency_keys.Remove(in_id);
}

bool GpuResourceManager::UploadTexture(
    SDL_GPUCommandBuffer* in_cmd,
    const asset::AssetId& in_id,
    const asset::Texture2D& in_texture
)
{
    if (in_texture.width == 0 || in_texture.height == 0 || in_texture.pixels.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "UploadTexture: Invalid texture dimensions or empty pixel data.");
        return false;
    }

    const bool compressed = asset::IsCompressed(in_texture.format);
    const bool has_mip_chain = !in_texture.mips.IsEmpty();

    // BCn 압축 포맷은 GPU가 밉맵을 생성할 수 없으므로 사전에 밉 체인이 있어야 함
    if (compressed && !has_mip_chain)
    {
        ConsoleLog(ELogLevel::Error, "UploadTexture: BCn compressed textures require a precomputed mip chain.");
        return false;
    }

    const SDL_GPUTextureFormat sdl_format = ToSDLFormat(in_texture.format);
    if (sdl_format == SDL_GPU_TEXTUREFORMAT_INVALID)
    {
        ConsoleLog(ELogLevel::Error, "UploadTexture: Unsupported ETextureFormat value.");
        return false;
    }

    // GPU가 밉맵을 자동 생성할지 여부
    const bool gpu_gen_mips = in_texture.generate_mips && !compressed && !has_mip_chain;

    // Mipmap 레벨 수 결정
    const uint32 num_levels = [&] -> uint32
    {
        if (has_mip_chain)
        {
            return static_cast<uint32>(in_texture.mips.Len());
        }

        if (gpu_gen_mips)
        {
            // 1 + floor(log2(max(w, h)))
            return static_cast<uint32>(
                std::floor(std::log2(static_cast<double>(std::max(in_texture.width, in_texture.height))))
            ) + 1;
        }

        return 1;
    }();

    SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    if (gpu_gen_mips)
    {
        // GPU 밉 생성(Blit)을 위해 COLOR_TARGET 플래그 필요
        usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    }

    // GPU Texture 생성
    const SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = sdl_format,
        .usage = usage,
        .width = in_texture.width,
        .height = in_texture.height,
        .layer_count_or_depth = 1,
        .num_levels = num_levels,
        .sample_count = SDL_GPU_SAMPLECOUNT_1
    };

    const RID texture_rid = render_device->CreateTexture(create_info);
    if (!texture_rid.IsValid())
    {
        return false;
    }

    // Transfer Buffer 생성 (픽셀 전체 크기)
    const uint32 total_pixel_size = static_cast<uint32>(in_texture.pixels.Len());
    const SDL_GPUTransferBufferCreateInfo transfer_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = total_pixel_size
    };

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(render_device->GetRawDevice(), &transfer_info);
    if (!transfer_buffer)
    {
        render_device->DestroyTexture(texture_rid);
        ConsoleLog(ELogLevel::Error, "UploadTexture: Failed to create transfer buffer: {}", SDL_GetError());
        return false;
    }

    // 메모리 맵핑 및 복사 (CPU -> Transfer Buffer)
    if (void* mapped_ptr = SDL_MapGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer, false))
    {
        std::memcpy(mapped_ptr, in_texture.pixels.Data(), total_pixel_size);
        SDL_UnmapGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);
    }
    else
    {
        render_device->DestroyTexture(texture_rid);
        SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);
        ConsoleLog(ELogLevel::Error, "UploadTexture: Failed to map transfer buffer: {}", SDL_GetError());
        return false;
    }

    // Raw Pointer 가져오기
    SDL_GPUTexture* texture = render_device->GetTexture(texture_rid)->handle;

    // GPU에 업로드
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(in_cmd);
    if (has_mip_chain)
    {
        // 사전 계산된 각 밉 레벨을 순서대로 업로드
        for (const auto [mip_level, desc] : in_texture.mips | std::views::enumerate)
        {
            SE_ASSERT(desc.offset + desc.size <= total_pixel_size);

            const SDL_GPUTextureTransferInfo src_info = {
                .transfer_buffer = transfer_buffer,
                .offset = desc.offset,
                .pixels_per_row = desc.width,
                .rows_per_layer = desc.height
            };

            const SDL_GPUTextureRegion dst_region = {
                .texture = texture,
                .mip_level = static_cast<uint32>(mip_level),
                .w = desc.width,
                .h = desc.height,
                .d = 1
            };

            SDL_UploadToGPUTexture(copy_pass, &src_info, &dst_region, false);
        }
    }
    else
    {
        // 밉 0만 업로드
        const SDL_GPUTextureTransferInfo src_info = {
            .transfer_buffer = transfer_buffer,
            .offset = 0,
            .pixels_per_row = in_texture.width,
            .rows_per_layer = in_texture.height
        };

        const SDL_GPUTextureRegion dst_region = {
            .texture = texture,
            .mip_level = 0,
            .w = in_texture.width,
            .h = in_texture.height,
            .d = 1
        };

        SDL_UploadToGPUTexture(copy_pass, &src_info, &dst_region, false);
    }
    SDL_EndGPUCopyPass(copy_pass);

    // 밉맵 생성 (업로드 직후 수행)
    if (gpu_gen_mips && num_levels > 1)
    {
        SDL_GenerateMipmapsForGPUTexture(in_cmd, texture);
    }

    SDL_ReleaseGPUTransferBuffer(render_device->GetRawDevice(), transfer_buffer);

    texture_map.Insert(in_id, texture_rid);
    return true;
}

Optional<TextureResource> GpuResourceManager::GetTexture(const asset::AssetId& in_id) const
{
    return texture_map
        .Find(in_id)
        .AndThen([this](const RID& rid)
        {
            return render_device->GetTexture(rid);
        });
}

void GpuResourceManager::UnloadTexture(const asset::AssetId& in_id)
{
    if (const Optional<RID> texture_rid = texture_map.Find(in_id))
    {
        render_device->DestroyTexture(*texture_rid);
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
    geometry_blocks.Emplace(render_device, new_block_size, usage);
    GpuMemoryBlock& new_block = geometry_blocks.Back().Value();

    GpuBufferSlice slice;
    [[maybe_unused]] const bool result = new_block.AllocateSlice(in_size, alignment, slice);
    SE_ASSERT(result); // 방금 만든 블록이므로 반드시 성공해야 함

    return slice;
}
}  // namespace se::graphics
