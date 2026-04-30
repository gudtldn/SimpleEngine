// NOLINTBEGIN(*-reserved-identifier)
#include "AssimpMaterialExtractor.h"

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineMaterialNode.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Utility/SHA256.h"

#include "assimp/material.h"
#include "assimp/scene.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
namespace
{
/**
 * TextureExtractionResult에서 aiMaterial 텍스처 슬롯의 GUID를 조회합니다.
 * embedded ref(*N) 또는 외부 경로 모두 처리합니다.
 */
Optional<Guid> FindTextureGuid(
    const aiMaterial* mat,
    aiTextureType tex_type,
    const TextureExtractionResult& texture_result,
    const Path& source_dir
)
{
    if (mat->GetTextureCount(tex_type) == 0)
    {
        return NullOpt;
    }

    aiString ai_path;
    if (mat->GetTexture(tex_type, 0, &ai_path) != AI_SUCCESS)
    {
        return NullOpt;
    }

    const StringView path_sv = ai_path.C_Str();

    // embedded ref: "*N" 형태
    if (path_sv.ByteLen() > 0 && path_sv[0] == '*')
    {
        const uint32 idx = static_cast<uint32>(std::strtoul(ai_path.C_Str() + 1, nullptr, 10));
        if (const Optional guid_opt = texture_result.embedded_index_to_guid.Find(idx))
        {
            return *guid_opt;
        }
        return NullOpt;
    }

    // 경로 traversal 차단
    if (path_sv.Contains(".."))
    {
        return NullOpt;
    }

    // 외부 경로: source_dir 기준으로 정규화
    const Path norm_path = source_dir.IsEmpty()
        ? Path{ ai_path.C_Str() }
        : source_dir / ai_path.C_Str();

    if (const Optional guid_opt = texture_result.path_to_guid.Find(norm_path))
    {
        return *guid_opt;
    }

    return NullOpt;
}
} // namespace


Guid AssimpMaterialExtractor::MakeDeterministicGuid(const String& key)
{
    // SHA-256(key) 첫 16바이트를 UUID 포맷으로 조합합니다.
    // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (8-4-4-4-12 hex chars)
    const ContentHash hash = SHA256::HashString(key);
    const uint8* d = hash.Data();

    const String uuid_str = String::Format(
        "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
        d[0],  d[1],  d[2],  d[3],
        d[4],  d[5],
        d[6],  d[7],
        d[8],  d[9],
        d[10], d[11], d[12], d[13], d[14], d[15]
    );

    return Guid::FromString(uuid_str);
}


AssimpMaterialExtractor::MaterialExtractionResult AssimpMaterialExtractor::ExtractMaterialsFromScene(
    const Path& file_path,
    const aiScene* scene,
    const TextureExtractionResult& texture_result,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("AssimpMaterialExtractor::ExtractMaterialsFromScene");

    MaterialExtractionResult result;

    if (!scene || scene->mNumMaterials == 0)
    {
        return result;
    }

    result.material_index_to_guid.Reserve(scene->mNumMaterials);

    const Path source_dir = file_path.Parent().ValueOr(Path{});
    const String file_str = file_path.ToString();

    for (uint32 mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx)
    {
        const aiMaterial* mat = scene->mMaterials[mat_idx];

        // 결정론적 GUID: "{file_path}_Mat_{mat_name}" 해싱
        aiString ai_mat_name;
        mat->Get(AI_MATKEY_NAME, ai_mat_name);
        const String mat_name = ai_mat_name.length > 0
            ? String{ ai_mat_name.C_Str() }
            : String::Format("Mat_{}", mat_idx);

        const String key = String::Format("{}_Mat_{}_{}", file_str, mat_name, mat_idx);
        const Guid mat_guid = MakeDeterministicGuid(key);

        // PipelineMaterialNode 생성 및 UID 설정
        PipelineMaterialNode& node = out_container.CreateNode<PipelineMaterialNode>();
        node.SetUid(mat_guid);
        node.SetDisplayName(mat_name);

        // --- Base Color Texture ---
        // glTF PBR: aiTextureType_BASE_COLOR, FBX legacy: aiTextureType_DIFFUSE
        Optional<Guid> base_color_tex = FindTextureGuid(mat, aiTextureType_BASE_COLOR, texture_result, source_dir);
        if (!base_color_tex.HasValue())
        {
            base_color_tex = FindTextureGuid(mat, aiTextureType_DIFFUSE, texture_result, source_dir);
        }

        if (base_color_tex.HasValue())
        {
            node.SetBaseColorTexture(*base_color_tex);
        }
        else
        {
            // 텍스처 없으면 diffuse 컬러 폴백
            aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            node.SetBaseColorValue(Vector4{ diffuse.r, diffuse.g, diffuse.b, diffuse.a });
        }

        // --- Normal Map ---
        if (Optional<Guid> tex = FindTextureGuid(mat, aiTextureType_NORMALS, texture_result, source_dir))
        {
            node.SetNormalTexture(*tex);
        }

        // --- Roughness ---
        if (Optional<Guid> tex = FindTextureGuid(mat, aiTextureType_DIFFUSE_ROUGHNESS, texture_result, source_dir))
        {
            node.SetRoughnessTexture(*tex);
        }
        else
        {
            float roughness = 0.5f;
            mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            node.SetRoughnessValue(roughness);
        }

        // --- Metallic ---
        if (Optional<Guid> tex = FindTextureGuid(mat, aiTextureType_METALNESS, texture_result, source_dir))
        {
            node.SetMetallicTexture(*tex);
        }
        else
        {
            float metallic = 0.0f;
            mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
            node.SetMetallicValue(metallic);
        }

        // --- Emissive ---
        if (Optional<Guid> tex = FindTextureGuid(mat, aiTextureType_EMISSIVE, texture_result, source_dir))
        {
            node.SetEmissiveTexture(*tex);
        }
        else
        {
            aiColor3D emissive(0.0f, 0.0f, 0.0f);
            mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
            if (emissive.r > 0.0f || emissive.g > 0.0f || emissive.b > 0.0f)
            {
                node.SetEmissiveValue(Vector3{ emissive.r, emissive.g, emissive.b });
            }
        }

        // --- Alpha Mode ---
        aiString alpha_mode_str;
        if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode_str) == AI_SUCCESS)
        {
            const StringView alpha_mode{ alpha_mode_str.C_Str() };
            if (alpha_mode == "MASK")
            {
                node.SetBlendMode(graphics::EBlendMode::Masked);
                float cutoff = 0.5f;
                mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff);
                node.SetAlphaCutoff(cutoff);
            }
            else if (alpha_mode == "BLEND")
            {
                node.SetBlendMode(graphics::EBlendMode::Translucent);
            }
        }

        // --- Two Sided ---
        int two_sided = 0;
        if (mat->Get(AI_MATKEY_TWOSIDED, two_sided) == AI_SUCCESS && two_sided != 0)
        {
            node.SetTwoSided(true);
        }

        result.material_index_to_guid.Push(mat_guid);
    }

    return result;
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
