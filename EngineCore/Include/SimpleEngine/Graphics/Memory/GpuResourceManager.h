#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Graphics/Memory/GpuBufferSlice.h"
#include "SimpleEngine/Graphics/Memory/GpuMemoryBlock.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
/**
 * GPU의 영구적 Resource(Mesh Buffer, Texture)을 관리하는 중앙 매니저
 *
 * - Mesh 데이터는 거대한 버퍼 블록(Page)에 선형 할당(Sub-allocation)되어 관리됩니다.
 * - Texture 데이터는 개별 SDL_GPUTexture 객체로 생성되어 관리됩니다.
 */
class SE_CORE_API GpuResourceManager
{
public:
    /** GPU에 업로드된 메시의 소스/설정 해시 쌍 (Hot-reload 감지용) */
    struct MeshResidencyKey
    {
        ContentHash source_hash;
        ContentHash settings_hash;

        bool operator==(const MeshResidencyKey&) const = default;
    };

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
     * @return 성공 시 true, 실패 시 false
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
     * AssetId에 매핑된 GPU 버퍼 슬라이스를 반환합니다.
     * @param in_id Asset ID
     * @return 유효한 GpuBufferSlice, 찾지 못하면 NullOpt 반환
     */
    [[nodiscard]] Optional<const GpuBufferSlice&> GetSlice(const asset::AssetId& in_id) const;

    /**
     * 메시의 Residency Key를 반환합니다. (Hot-reload 감지용)
     * @param in_id Asset ID
     * @return 키가 존재하면 Optional 반환, 없으면 NullOpt
     */
    [[nodiscard]] Optional<const MeshResidencyKey&> GetMeshResidencyKey(const asset::AssetId& in_id) const;

    /**
     * 메시의 Residency Key를 설정합니다.
     * @param in_id Asset ID
     * @param in_key 설정할 MeshResidencyKey
     */
    void SetMeshResidencyKey(const asset::AssetId& in_id, MeshResidencyKey in_key);

    /**
     * 메시의 Residency Key를 제거합니다. UnloadMesh 호출 시 함께 호출하세요.
     * @param in_id Asset ID
     */
    void RemoveMeshResidencyKey(const asset::AssetId& in_id);

    /**
     * Texture2D 에셋을 GPU Texture로 업로드하고 관리 목록에 등록합니다.
     * format, mip 체인, generate_mips 정보는 에셋에서 자동으로 읽습니다.
     * @param in_cmd Upload 명령을 기록할 Command Buffer
     * @param in_id Asset ID
     * @param in_texture 업로드할 Texture2D 에셋
     * @return 성공 시 true, 실패 시 false
     */
    bool UploadTexture(
        SDL_GPUCommandBuffer* in_cmd,
        const asset::AssetId& in_id,
        const asset::Texture2D& in_texture
    );

    /**
     * AssetId에 매핑된 GPU Texture 객체 정보를 반환합니다.
     * @param in_id Asset ID
     * @return 유효한 TextureResource, 찾지 못하면 NullOpt 반환
     */
    [[nodiscard]] Optional<const TextureResource&> GetTexture(const asset::AssetId& in_id) const;

    /**
     * 특정 텍스처를 GPU 메모리에서 즉시 해제합니다.
     * @param in_id 해제할 Asset ID
     */
    void UnloadTexture(const asset::AssetId& in_id);

private:
    // 기본 블록 크기 (128MB)
    static constexpr uint32 DEFAULT_BLOCK_SIZE = 128 * 1024 * 1024;

    /** 사용 가능한 Geometry Block을 찾아 메모리를 할당합니다. */
    [[nodiscard]] GpuBufferSlice AllocateInGeometryBlock(uint32 in_size);

    // TODO: 추후 Unload한 Mesh에 대해서 Defragmentation을 적용 후 VRAM 최적화

private:
    RenderDevice* render_device;

    // Geometry(Vertex+Index)용 메모리 블록 목록
    Array<GpuMemoryBlock> geometry_blocks;

    // AssetId -> GPU 메모리 위치 매핑
    HashMap<asset::AssetId, GpuBufferSlice> slice_map;

    // AssetId -> MeshResidencyKey 매핑 (Hot-reload 감지용)
    HashMap<asset::AssetId, MeshResidencyKey> mesh_residency_keys;

    // AssetId -> GPU Texture 매핑
    HashMap<asset::AssetId, RID> texture_map;
};
}  // namespace se::graphics
