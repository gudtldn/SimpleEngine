// NOLINTBEGIN(*-reserved-identifier)
#include "AssimpTextureExtractor.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineTextureNode.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "assimp/material.h"
#include "assimp/scene.h"
#include "assimp/texture.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
TextureExtractionResult AssimpTextureExtractor::ExtractTexturesFromScene(
    const aiScene* scene,
    const Path& source_file,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("AssimpTextureExtractor::ExtractTexturesFromScene");

    TextureExtractionResult result;

    if (!scene)
    {
        return result;
    }

    const Optional<Path> source_dir = source_file.Parent();
    const String stem = source_file.FileStem().ValueOr("tex");

    // -------------------------------------------------------------------------
    // Part A: Embedded textures (scene->mTextures[i])
    // -------------------------------------------------------------------------
    for (uint32 i = 0; i < scene->mNumTextures; ++i)
    {
        const aiTexture* tex = scene->mTextures[i];

        PipelineTextureNode& node = out_container.CreateNode<PipelineTextureNode>();
        node.SetDisplayName(String::Format("{}_tex_{}", stem, i));
        node.SetSRGB(true); // TODO: slot별 override 적용하기

        if (tex->mHeight == 0)
        {
            // 압축 바이너리 (PNG/JPG 등)
            const usize byte_count = static_cast<usize>(tex->mWidth);
            if (byte_count == 0)
            {
                ConsoleLog(ELogLevel::Warning, "AssimpTextureExtractor: Embedded texture [{}] has zero byte count, skipping.", i);
                continue;
            }
            Array<uint8> bytes;
            bytes.ResizeUninitialized(byte_count);
            std::memcpy(bytes.Data(), tex->pcData, byte_count);

            node.SetEmbeddedBytes(std::move(bytes));
            node.SetEmbeddedFormat(tex->achFormatHint);
        }
        else
        {
            // Raw ARGB8 aiTexel -> RGBA8 변환
            // aiTexel 필드: r, g, b, a (각 uint8)
            const usize pixel_count = static_cast<usize>(tex->mWidth) * tex->mHeight;
            Array<uint8> rgba8;
            rgba8.ResizeUninitialized(pixel_count * 4u);

            for (usize j = 0; j < pixel_count; ++j)
            {
                rgba8[(j * 4) + 0] = tex->pcData[j].r;
                rgba8[(j * 4) + 1] = tex->pcData[j].g;
                rgba8[(j * 4) + 2] = tex->pcData[j].b;
                rgba8[(j * 4) + 3] = tex->pcData[j].a;
            }

            node.SetEmbeddedBytes(std::move(rgba8));
            node.SetEmbeddedWidth(static_cast<uint64>(tex->mWidth));
            node.SetEmbeddedHeight(static_cast<uint64>(tex->mHeight));
        }

        result.embedded_index_to_guid.Insert(i, node.GetUid());
    }

    // -------------------------------------------------------------------------
    // Part B: External textures (aiMaterial 순회)
    // -------------------------------------------------------------------------
    static constexpr aiTextureType TEXTURE_TYPES[] = {
        aiTextureType_DIFFUSE,
        aiTextureType_NORMALS,
        aiTextureType_BASE_COLOR,
        aiTextureType_METALNESS,
        aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_SPECULAR,
        aiTextureType_EMISSIVE,
        aiTextureType_AMBIENT_OCCLUSION,
    };

    HashSet<Path> seen_paths;

    for (uint32 mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx)
    {
        const aiMaterial* mat = scene->mMaterials[mat_idx];

        for (const aiTextureType tex_type : TEXTURE_TYPES)
        {
            const uint32 tex_count = mat->GetTextureCount(tex_type);
            for (uint32 tex_idx = 0; tex_idx < tex_count; ++tex_idx)
            {
                aiString ai_path;
                if (mat->GetTexture(tex_type, tex_idx, &ai_path) != AI_SUCCESS)
                {
                    continue;
                }

                // embedded ref (예: "*0")는 이미 Part A에서 처리 함
                if (ai_path.length > 0 && ai_path.data[0] == '*')
                {
                    continue;
                }

                // 경로 traversal 차단 ("../" 포함 경로는 FBX 소스 디렉토리 밖을 참조할 수 있음)
                const StringView ai_path_sv = ai_path.C_Str();
                if (ai_path_sv.Contains(".."))
                {
                    ConsoleLog(ELogLevel::Warning, "AssimpTextureExtractor: Blocked potentially unsafe texture path: {}", ai_path_sv);
                    continue;
                }

                // 경로 정규화
                const Path norm_path = source_dir.HasValue()
                    ? (*source_dir / ai_path.C_Str())
                    : Path{ ai_path.C_Str() };

                if (seen_paths.Contains(norm_path))
                {
                    continue;
                }
                seen_paths.Insert(norm_path);

                PipelineTextureNode& node = out_container.CreateNode<PipelineTextureNode>();
                node.SetDisplayName(norm_path.FileStem().ValueOr(norm_path.ToString()));
                node.SetSourceFile(norm_path);
                node.SetSRGB(true); // TODO: slot별 override 적용하기

                result.path_to_guid.Insert(norm_path, node.GetUid());
            }
        }
    }

    return result;
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
