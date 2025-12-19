#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Rendering/Memory/GpuBufferSlice.h"
#include "SimpleEngine/Rendering/Memory/GpuMemoryBlock.h"

#include "SDL3/SDL_gpu.h"


namespace se::rendering
{
/**
 * @todo docs
 */
class GpuResourceManager
{
public:
    explicit GpuResourceManager(SDL_GPUDevice* in_device);
    ~GpuResourceManager();

    // 이동 & 복사 생성자 제거
    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;
    GpuResourceManager(GpuResourceManager&&) noexcept = delete;
    GpuResourceManager& operator=(GpuResourceManager&&) noexcept = delete;

public:
    /** @todo docs */
    bool UploadMesh(
        const asset::AssetId& in_id,
        const void* in_vertex_data, uint32 in_vertex_size,
        const void* in_index_data, uint32 in_index_size
    );

    /** @todo docs */
    [[nodiscard]] const GpuBufferSlice& GetSlice(const asset::AssetId& in_id) const;

private:
    // 기본 블록 크기 (128MB)
    static constexpr uint32 DEFAULT_BLOCK_SIZE = 128 * 1024 * 1024;
    static constexpr GpuBufferSlice EmptySlice;

    GpuBufferSlice AllocateInGeometryBlock(uint32 in_size);

private:
    SDL_GPUDevice* device = nullptr;

    // Geometry(Vertex+Index)용 메모리 블록 목록
    Array<GpuMemoryBlock> geometry_blocks;

    // AssetId -> GPU 메모리 위치 매핑
    HashMap<asset::AssetId, GpuBufferSlice> slice_map;
};
}  // namespace se::rendering
