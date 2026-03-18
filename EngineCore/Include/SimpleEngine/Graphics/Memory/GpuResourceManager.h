#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Graphics/Memory/GpuBufferSlice.h"
#include "SimpleEngine/Graphics/Memory/GpuMemoryBlock.h"
#include "SimpleEngine/Graphics/Memory/GpuTexture.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
// forward declaration
class RenderDevice;

/** 텍스처 업로드 시 적용할 옵션 */
struct TextureUploadSettings
{
    /**
     * sRGB 포맷 사용 여부
     * - true: Albedo(BaseColor) 등 색상 데이터에 사용 (SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB)
     * - false: Normal, Roughness, Metallic 등 비색상 데이터에 사용 (SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM)
     */
    bool is_srgb = false;

    /**
     * 밉맵(Mipmap) 생성 여부
     * - true: 텍스처 업로드 후 자동으로 밉맵 체인을 생성합니다. (메모리 사용량 약 33% 증가)
     * - false: 밉맵을 생성하지 않습니다. (UI 텍스처 등)
     */
    bool generate_mips = true;

    // bool compress = false; // 추후 압축 텍스처(BCn/ASTC) 지원 시 사용 예정
};

/**
 * GPU의 영구적 Resource(Mesh Buffer, Texture)을 관리하는 중앙 매니저
 *
 * - Mesh 데이터는 거대한 버퍼 블록(Page)에 선형 할당(Sub-allocation)되어 관리됩니다.
 * - Texture 데이터는 개별 SDL_GPUTexture 객체로 생성되어 관리됩니다.
 */
class SE_CORE_API GpuResourceManager
{
public:
    explicit GpuResourceManager(RenderDevice& in_render_device);
    ~GpuResourceManager();

    // 이동 & 복사 생성자 제거
    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;
    GpuResourceManager(GpuResourceManager&&) noexcept = delete;
    GpuResourceManager& operator=(GpuResourceManager&&) noexcept = delete;

public:
    /**
     * Mesh 데이터를 GPU 버퍼로 업로드하고 관리 목록에 등록합니다.
     * @param in_cmd Upload 명령을 기록할 Command Buffer
     * @param in_id Asset ID
     * @param in_vertex_data Vertex 데이터 포인터
     * @param in_vertex_size Vertex 데이터 전체 크기 (bytes)
     * @param in_index_data Index 데이터 포인터
     * @param in_index_size Index 데이터 전체 크기 (bytes)
     * @return 성공 시 true, 실패 시 false (VRAM 부족 등)
     */
    bool UploadMesh(
        SDL_GPUCommandBuffer* in_cmd,
        const asset::AssetId& in_id,
        const void* in_vertex_data, uint32 in_vertex_size,
        const void* in_index_data, uint32 in_index_size
    );

    /**
     * 등록된 Mesh 정보를 해제합니다.
     * @note 현재 구현은 Linear Allocation(Bump Pointer)을 사용하므로, 이 함수를 호출해도 실제 VRAM이 반환되지는 않고,
     *       단순 slice_map(AssetId 매핑)에서 제거되어 접근을 막는 함수 입니다.
     * @param in_id 해제할 Asset ID
     */
    void UnloadMesh(const asset::AssetId& in_id);

    /**
     * AssetId에 매핑된 GPU 버퍼 슬라이스(위치 정보)를 반환합니다.
     *
     * @param in_id Asset ID
     * @return 유효한 GpuBufferSlice, 찾지 못하면 EmptySlice 반환
     */
    [[nodiscard]] const GpuBufferSlice& GetSlice(const asset::AssetId& in_id) const;

    /**
     * CPU Surface(이미지)를 GPU Texture로 변환하여 업로드합니다.
     * 입력된 Surface는 내부적으로 GPU 호환 포맷(RGBA32)으로 변환되어 업로드됩니다.
     *
     * @param in_cmd Upload 명령을 기록할 Command Buffer
     * @param in_id Asset ID
     * @param in_surface SDL_Surface 포인터 (이미지 데이터)
     * @param in_settings 텍스처 설정 (sRGB 포맷 여부, Mipmap 생성 여부 등)
     * @return 성공 시 true, 실패 시 false
     */
    bool UploadTexture(
        SDL_GPUCommandBuffer* in_cmd,
        const asset::AssetId& in_id,
        const SDL_Surface* in_surface,
        TextureUploadSettings in_settings = TextureUploadSettings{}
    );

    /**
     * AssetId에 매핑된 GPU Texture 객체 정보를 반환합니다.
     *
     * @param in_id Asset ID
     * @return 유효한 GpuTexture, 찾지 못하면 EmptyTexture 반환
     */
    [[nodiscard]] const GpuTexture& GetTexture(const asset::AssetId& in_id) const;

    /**
     * 특정 텍스처를 GPU 메모리에서 즉시 해제합니다.
     * @param in_id 해제할 Asset ID
     */
    void UnloadTexture(const asset::AssetId& in_id);

private:
    // 기본 블록 크기 (128MB)
    static constexpr uint32 DEFAULT_BLOCK_SIZE = 128 * 1024 * 1024;
    static constexpr GpuBufferSlice EmptySlice{};
    static constexpr GpuTexture EmptyTexture{};

    /** 사용 가능한 Geometry Block을 찾아 메모리를 할당합니다. */
    [[nodiscard]] GpuBufferSlice AllocateInGeometryBlock(uint32 in_size);

    // TODO: 추후 Unload한 Mesh에 대해서 Defragmentation을 적용 후 VRAM 최적화

private:
    RenderDevice* render_device;

    // Geometry(Vertex+Index)용 메모리 블록 목록
    Array<GpuMemoryBlock> geometry_blocks;

    // AssetId -> GPU 메모리 위치 매핑
    HashMap<asset::AssetId, GpuBufferSlice> slice_map;

    // AssetId -> GPU Texture 매핑
    HashMap<asset::AssetId, GpuTexture> texture_map;
};
}  // namespace se::graphics
