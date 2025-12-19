#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Rendering/Memory/GpuBufferSlice.h"
#include "SimpleEngine/Rendering/Memory/GpuMemoryBlock.h"
#include "SimpleEngine/Rendering/Memory/GpuTexture.h"

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
    /** Mesh Data를 GPU로 업로드합니다. */
    bool UploadMesh(
        const asset::AssetId& in_id,
        const void* in_vertex_data, uint32 in_vertex_size,
        const void* in_index_data, uint32 in_index_size
    );

    /** @todo docs */
    [[nodiscard]] const GpuBufferSlice& GetSlice(const asset::AssetId& in_id) const;

    /**
     * CPU Surface(이미지)를 GPU Texture로 업로드합니다.
     * @todo 추후 밉맵 생성여부도 확인
     * @param in_id AssetId
     * @param in_surface SDL_Surface 포인터 (이미지 데이터)
     */
    bool UploadTexture(const asset::AssetId& in_id, const SDL_Surface* in_surface);

    [[nodiscard]] const GpuTexture& GetTexture(const asset::AssetId& in_id) const;

    /** 특정 텍스처를 메모리에서 해제합니다. */
    void UnloadTexture(const asset::AssetId& in_id);

private:
    // 기본 블록 크기 (128MB)
    static constexpr uint32 DEFAULT_BLOCK_SIZE = 128 * 1024 * 1024;
    static constexpr GpuBufferSlice EmptySlice{};
    static constexpr GpuTexture EmptyTexture{};

    [[nodiscard]] GpuBufferSlice AllocateInGeometryBlock(uint32 in_size);

    // TODO: 나중에 사용하지 않는 버퍼 해제하는 로직 추가

private:
    SDL_GPUDevice* device = nullptr;

    // Geometry(Vertex+Index)용 메모리 블록 목록
    Array<GpuMemoryBlock> geometry_blocks;

    // AssetId -> GPU 메모리 위치 매핑
    HashMap<asset::AssetId, GpuBufferSlice> slice_map;

    // AssetId -> GPU Texture 매핑
    HashMap<asset::AssetId, GpuTexture> texture_map;
};
}  // namespace se::rendering
