#pragma once

#include "AssimpTextureExtractor.h"

#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/Path.h"

// forward declarations
struct aiScene;


namespace se::editor
{
/**
 * Assimp aiScene에서 머티리얼 정보를 추출하여
 * PipelineMaterialNode를 생성하는 유틸리티 클래스
 */
class AssimpMaterialExtractor
{
public:
    AssimpMaterialExtractor() = delete;

    struct MaterialExtractionResult
    {
        /** aiMaterial 인덱스 -> PipelineMaterialNode의 GUID */
        Array<Guid> material_index_to_guid;
    };

    /**
     * aiScene의 모든 머티리얼을 PipelineMaterialNode로 변환하여 out_container에 추가합니다.
     *
     * 결정론적 GUID: "{file_path}_Mat_{ai_mat_name}" 문자열을 SHA-256으로 해싱하여
     * 재임포트 시에도 동일한 GUID를 보장합니다.
     *
     * @param file_path 원본 파일 경로 (결정론적 GUID 생성에 사용)
     * @param scene Assimp 씬 포인터
     * @param texture_result ExtractTexturesFromScene의 반환값 (텍스처 GUID 조회용)
     * @param out_container 생성된 노드가 추가될 컨테이너
     * @return aiMaterial 인덱스 -> PipelineMaterialNode GUID 맵
     */
    [[nodiscard]] static MaterialExtractionResult ExtractMaterialsFromScene(
        const Path& file_path,
        const aiScene* scene,
        const TextureExtractionResult& texture_result,
        PipelineNodeContainer& out_container
    );

private:
    /** "{file_path}_Mat_{mat_name}" 문자열을 SHA-256 해싱하여 결정론적 Guid를 생성합니다. */
    [[nodiscard]] static Guid MakeDeterministicGuid(const String& key);
};
} // namespace se::editor
