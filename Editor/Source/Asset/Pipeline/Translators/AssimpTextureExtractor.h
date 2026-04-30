#pragma once

#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/Path.h"

// forward declarations
struct aiScene;


namespace se::editor
{
/**
 * AssimpTextureExtractor::ExtractTexturesFromScene의 반환값
 */
struct TextureExtractionResult
{
    /** scene->mTextures[i] 인덱스 -> PipelineTextureNode GUID */
    HashMap<uint32, Guid> embedded_index_to_guid;

    /** 정규화된 외부 텍스처 경로 -> PipelineTextureNode GUID */
    HashMap<Path, Guid> path_to_guid;
};

/**
 * Assimp aiScene에서 embedded/external 텍스처를 추출하여
 * PipelineTextureNode를 생성하는 유틸리티 클래스
 */
class AssimpTextureExtractor
{
public:
    AssimpTextureExtractor() = delete;

    /**
     * aiScene의 모든 텍스처를 PipelineTextureNode로 변환하여 out_container에 추가합니다.
     *
     * Part A: scene->mTextures[] (embedded textures)
     *   - mHeight == 0: 압축 바이너리 -> EMBEDDED_BYTES + EMBEDDED_FORMAT
     *   - mHeight > 0:  raw aiTexel -> RGBA8 변환 후 EMBEDDED_BYTES + EMBEDDED_WIDTH + EMBEDDED_HEIGHT
     *
     * Part B: aiMaterial 순회 -> 외부 텍스처 경로 추출 -> SOURCE_FILE
     *
     * @param scene Assimp 씬 포인터
     * @param source_file FBX 원본 파일 경로 (외부 텍스처 경로 기준점)
     * @param out_container 생성된 노드가 추가될 컨테이너
     * @return embedded/external 텍스처 GUID 맵
     */
    [[nodiscard]] static TextureExtractionResult ExtractTexturesFromScene(
        const aiScene* scene,
        const Path& source_file,
        PipelineNodeContainer& out_container
    );
};
} // namespace se::editor
