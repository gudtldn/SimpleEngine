#pragma once

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Graphics/Memory/GpuResourceManager.h"
#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::graphics
{
/**
 * 게임 스레드에서 준비한 메시 GPU 업로드 요청
 * AssetHandle을 보관하여 렌더 스레드가 GPU 업로드를 수행하는 동안 에셋이 Evict되지 않도록 합니다.
 */
struct MeshUploadRequest
{
    asset::AssetId mesh_id;
    asset::AssetHandle<asset::StaticMesh> handle;
    GpuResourceManager::MeshResidencyKey new_residency_key;
};

/**
 * 게임 스레드에서 준비한 텍스처 GPU 업로드 요청
 * AssetHandle을 보관하여 렌더 스레드가 GPU 업로드를 수행하는 동안 에셋이 Evict되지 않도록 합니다.
 */
struct TextureUploadRequest
{
    asset::AssetId texture_id;
    asset::AssetHandle<asset::Texture2D> handle;
};

/**
 * 한 프레임의 렌더링에 필요한 모든 데이터를 묶는 최상위 구조체
 * @note Application 레이어에서 생성되어, RenderSubsystem에 전달됩니다.
 */
struct FramePacket
{
    Array<RenderView> render_views;
    SceneDrawData scene_draw_data;
    uint64 frame_number = 0;

    // GPU에 업로드 할 Mesh/Material 목록
    Array<MeshUploadRequest> mesh_upload_requests;
    Array<TextureUploadRequest> texture_upload_requests;

    // FramePacket 수명 동안 MaterialInstance가 Evict되지 않도록 Handle을 소유
    Array<asset::AssetHandle<asset::MaterialInstance>> pinned_material_handles;
};
} // namespace se::graphics
