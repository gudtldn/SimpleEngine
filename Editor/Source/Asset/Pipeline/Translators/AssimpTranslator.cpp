// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/Translators/AssimpTranslator.h"

#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineMaterialInstanceNode.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineTextureNode.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"

#include "assimp/config.h"
#include "assimp/GltfMaterial.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "tracy/Tracy.hpp"

#include <bit>
#include <charconv>


namespace
{
using namespace se;
using namespace se::editor;

// ---------------------------------------------------------------------------
// Y-up RH -> Z-up RH 좌표계 변환 행렬
// 변환: (x, y, z) -> (x, -z, y)  |  X축 기준 -90도 회전
// det = +1 (순수 회전) -> face winding 변경 불필요
// ---------------------------------------------------------------------------

// Assimp column-vector convention (v' = M * v), row-major 저장
const aiMatrix4x4 YUP_TO_ZUP_COL = {
    1,  0,  0,  0,
    0,  0, -1,  0,
    0,  1,  0,  0,
    0,  0,  0,  1,
};

// Engine row-vector convention (v' = v * M), row-major 저장
constexpr Matrix4x4f YUP_TO_ZUP_ROW = {
    1,  0,  0,  0,
    0,  0,  1,  0,
    0, -1,  0,  0,
    0,  0,  0,  1,
};

// Z-up -> Y-up (역변환, 전치 = 역행렬 since orthogonal)
constexpr Matrix4x4f ZUP_TO_YUP_ROW = {
    1,  0,  0,  0,
    0,  0, -1,  0,
    0,  1,  0,  0,
    0,  0,  0,  1,
};

// ---------------------------------------------------------------------------
// 유틸리티 함수
// ---------------------------------------------------------------------------

Vector3f ToVec3f(const aiVector3D& v)
{
    return { v.x, v.y, v.z };
}

/** Bitangent sign 계산: tangent.w = sign(dot(cross(N, T), B)) */
float ComputeTangentSign(const Vector3f& normal, const Vector3f& tangent, const Vector3f& bitangent)
{
    return normal.Cross(tangent).Dot(bitangent) >= 0.0f ? 1.0f : -1.0f;
}

/**
 * aiMatrix4x4 (column-vector, Y-up) -> Matrix4x4f (row-vector, Z-up)
 *   1) Transpose: column-vec -> row-vec convention
 *   2) Similarity transform: C^(-1) * T_yup * C -> T_zup
 */
Matrix4x4f ConvertNodeTransform(const aiMatrix4x4& ai_mat)
{
    // Transpose for convention change (column-vec -> row-vec)
    const Matrix4x4f t_yup_row = {
        ai_mat.a1, ai_mat.b1, ai_mat.c1, ai_mat.d1,
        ai_mat.a2, ai_mat.b2, ai_mat.c2, ai_mat.d2,
        ai_mat.a3, ai_mat.b3, ai_mat.c3, ai_mat.d3,
        ai_mat.a4, ai_mat.b4, ai_mat.c4, ai_mat.d4
    };

    // T_zup = C^(-1) * T_yup * C (similarity transform)
    return ZUP_TO_YUP_ROW * t_yup_row * YUP_TO_ZUP_ROW;
}

/**
 * aiMesh에서 vertex 데이터 추출
 *   convert_to_zup=true: 변환 행렬을 적용하여 Y-up -> Z-up 변환 (비-PTV 경로)
 *   convert_to_zup=false: 직접 읽기 (PTV가 이미 root transform으로 Z-up 변환 완료)
 * @note PTV(Pre-Transform Vertices)
 */
void ExtractVertices(const aiMesh* mesh, bool convert_to_zup, Array<StaticVertex>& out_vertices)
{
    out_vertices.Reserve(out_vertices.Len() + mesh->mNumVertices);

    // 변환 행렬 적용 헬퍼: v_zup = Vector4f(v_yup, w) * YUP_TO_ZUP_ROW
    auto calc_transform = [](const Vector3f& v, float w)
    {
        return Vector3f{ Vector4f{ v, w } * YUP_TO_ZUP_ROW };
    };

    for (uint32 i = 0; i < mesh->mNumVertices; ++i)
    {
        StaticVertex vertex;

        // 위치(Position) 처리
        Vector3f pos = ToVec3f(mesh->mVertices[i]);
        vertex.position = convert_to_zup ? calc_transform(pos, 1.0f) : pos;

        // 법선(Normal) 처리
        if (mesh->HasNormals())
        {
            Vector3f nrm = ToVec3f(mesh->mNormals[i]);
            vertex.normal = convert_to_zup ? calc_transform(nrm, 0.0f) : nrm;
        }

        // 탄젠트(Tangent) & 바이탄젠트(Bitangent) 처리
        if (mesh->HasTangentsAndBitangents())
        {
            Vector3f t = ToVec3f(mesh->mTangents[i]);
            Vector3f b = ToVec3f(mesh->mBitangents[i]);

            if (convert_to_zup)
            {
                t = calc_transform(t, 0.0f);
                b = calc_transform(b, 0.0f);
            }

            // Normal은 위에서 이미 변환되었으므로 그대로 사용
            vertex.tangent = Vector4f{ t, ComputeTangentSign(vertex.normal, t, b) };
        }

        // UV(TexCoord) 처리
        if (mesh->HasTextureCoords(0))
        {
            vertex.tex_coord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }

        out_vertices.Push(vertex);
    }
}

void ExtractIndices(const aiMesh* mesh, uint32 vertex_offset, Array<uint32>& out_indices)
{
    out_indices.Reserve(out_indices.Len() + (mesh->mNumFaces * 3ULL));
    for (uint32 f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        for (uint32 j = 0; j < face.mNumIndices; ++j)
        {
            out_indices.Push(face.mIndices[j] + vertex_offset);
        }
    }
}

// ---------------------------------------------------------------------------
// 메쉬 처리 함수
// ---------------------------------------------------------------------------

/**
 * 씬의 모든 primitive를 하나의 StaticMeshPipelineNode로 병합합니다.
 * combine_meshes=true일 때 1개의 StaticMesh 에셋(1 draw call)을 생성합니다.
 *
 * PTV + root transform 주입에 의해 vertex 데이터는 이미 Z-up 상태입니다.
 *
 * 또한 1개의 material만 지원하며, primitive가 여러 material을 가지더라도 첫 번째 primitive의 material을 사용합니다.
 * 여러 material이 필요하면 combine_meshes=false를 사용하세요.
 */
void ProcessMergedMesh(
    const aiScene* scene,
    const String& mesh_name,
    ImportContext& io_ctx,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("ProcessMergedMesh");

    const Guid guid = io_ctx.AllocateSubAssetGuid(mesh_name);
    StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>(guid);
    pipeline_node.SetDisplayName(mesh_name);

    // 전체 크기 사전 계산 후 예약
    uint32 total_vertices = 0;
    uint32 total_indices = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        total_vertices += scene->mMeshes[i]->mNumVertices;
        total_indices += scene->mMeshes[i]->mNumFaces * 3;
    }
    pipeline_node.vertices.Reserve(total_vertices);
    pipeline_node.indices.Reserve(total_indices);
    pipeline_node.sections.Reserve(scene->mNumMeshes);

    // 모든 primitive를 하나의 버퍼에 병합 (이미 Z-up, convert=false)
    uint32 vertex_offset = 0;
    uint32 index_offset = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];
        ExtractVertices(mesh, false, pipeline_node.vertices);
        ExtractIndices(mesh, vertex_offset, pipeline_node.indices);

        const uint32 index_count = mesh->mNumFaces * 3;
        pipeline_node.sections.Push({
            .index_offset = index_offset,
            .index_count = index_count,
            .vertex_offset = (index_count > 0) ? 0 : static_cast<int32>(vertex_offset),
            .vertex_count = mesh->mNumVertices,
            .material_index = mesh->mMaterialIndex
        });

        vertex_offset += mesh->mNumVertices;
        index_offset += index_count;
    }
}

/**
 * 노드 계층을 순회하며 각 메쉬를 개별 PipelineNode로 생성합니다.
 *
 * convert_to_zup=true (비-PTV 경로): Y-up -> Z-up 변환을 수동 적용하고 노드 transform을 보존
 * convert_to_zup=false (PTV 경로): PTV가 이미 변환 완료했으므로 직접 읽기
 */
void ProcessNodeIterative(
    const aiNode* root_node,
    const aiScene* scene,
    bool convert_to_zup,
    ImportContext& io_ctx,
    PipelineNodeContainer& out_container
)
{
    HashMap<String, uint32> name_count;

    Array<const aiNode*> stack;
    stack.Push(root_node);

    while (Optional node_opt = stack.Pop())
    {
        const aiNode* node = *node_opt;

        for (uint32 i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            const String base_name = String::Format("{}_{}", node->mName.C_Str(), mesh->mName.C_Str());

            // 동일 이름의 노드가 여러 개일 경우 접미사로 구분하여 GUID 안정성 보장
            String node_name = base_name;
            if (const auto count = name_count.Find(base_name))
            {
                const uint32 n = *count + 1;
                name_count.Insert(base_name, n);
                node_name = String::Format("{}_{}", base_name, n);
            }
            else
            {
                name_count.Insert(base_name, 0);
            }

            const Guid guid = io_ctx.AllocateSubAssetGuid(node_name);
            StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>(guid);
            pipeline_node.SetDisplayName(node_name);

            ExtractVertices(mesh, convert_to_zup, pipeline_node.vertices);
            ExtractIndices(mesh, 0, pipeline_node.indices);
            pipeline_node.sections.Push({
                .index_offset = 0,
                .index_count = mesh->mNumFaces * 3,
                .vertex_offset = 0,
                .vertex_count = mesh->mNumVertices,
                .material_index = mesh->mMaterialIndex,
            });

            // 비-PTV 경로: 노드의 로컬 트랜스폼을 Z-up으로 변환하여 보존
            if (convert_to_zup)
            {
                pipeline_node.local_transform = ConvertNodeTransform(node->mTransformation);
            }
        }

        for (uint32 i = 0; i < node->mNumChildren; ++i)
        {
            stack.Push(node->mChildren[i]);
        }
    }
}

// ---------------------------------------------------------------------------
// 머티리얼/텍스처 처리 함수
// ---------------------------------------------------------------------------

/**
 * aiMaterial에서 지정된 텍스처 타입의 텍스처를 추출하여 PipelineTextureNode를 생성합니다.
 * @return 생성된 노드의 GUID, 텍스처가 없으면 Guid::None
 */
Guid ExtractTexture(
    const aiMaterial* ai_mat,
    aiTextureType tex_type,
    const StringView& slot_name,
    const String& mat_name,
    bool srgb,
    const Path& model_dir,
    const aiScene* scene,
    HashMap<uint32, Guid>& embedded_tex_guids,
    HashMap<String, Guid>& external_tex_guids,
    ImportContext& io_ctx,
    PipelineNodeContainer& out_container
)
{
    aiString tex_path_str;
    if (ai_mat->GetTexture(tex_type, 0, &tex_path_str) != AI_SUCCESS)
    {
        return Guid::None;
    }

    const StringView tex_path_sv = tex_path_str.C_Str();
    if (tex_path_sv.IsEmpty())
    {
        return Guid::None;
    }

    // 임베디드 텍스처: 경로가 "*<index>" 형태
    if (tex_path_sv.StartsWith('*'))
    {
        // 인덱스 파싱
        const Optional<uint32> embedded_idx_opt = [&] -> Optional<uint32>
        {
            uint32 val = 0;
            const char* start = tex_path_sv.Data() + 1;
            const char* end = start + (tex_path_sv.ByteLen() - 1);

            if (auto [ptr, ec] = std::from_chars(start, end, val); ec == std::errc{})
            {
                return val;
            }
            return NullOpt;
        }();

        if (!embedded_idx_opt.HasValue())
        {
            ConsoleLog(ELogLevel::Error, "Failed to parse embedded texture index.");
            return Guid::None;
        }

        // 유효성 검사 수행
        const uint32 embedded_idx = *embedded_idx_opt;
        if (embedded_idx >= scene->mNumTextures)
        {
            return Guid::None;
        }

        return embedded_tex_guids.Entry(embedded_idx).OrInsertWith([&] -> Guid
        {
            const aiTexture* ai_tex = scene->mTextures[embedded_idx];
            const String tex_name = String::Format("Texture_Embedded_{}", embedded_idx);
            const Guid tex_guid = io_ctx.AllocateSubAssetGuid(tex_name);

            PipelineTextureNode& tex_node = out_container.CreateNode<PipelineTextureNode>(tex_guid);
            tex_node.SetDisplayName(tex_name);
            tex_node.SetSRGB(srgb);

            if (ai_tex->mHeight > 0)
            {
                // 이미 디코딩된 RGBA8 raw pixels
                const usize byte_count = static_cast<usize>(ai_tex->mWidth) * static_cast<usize>(ai_tex->mHeight) * 4u;
                tex_node.SetEmbeddedBytes({ reinterpret_cast<const uint8*>(ai_tex->pcData), byte_count });
                tex_node.SetEmbeddedWidth(static_cast<uint64>(ai_tex->mWidth));
                tex_node.SetEmbeddedHeight(static_cast<uint64>(ai_tex->mHeight));
            }
            else
            {
                // 압축 바이너리 (PNG/JPG/TGA ...), mWidth == 바이트 수
                tex_node.SetEmbeddedBytes({ reinterpret_cast<const uint8*>(ai_tex->pcData), static_cast<usize>(ai_tex->mWidth) });
                if (ai_tex->achFormatHint[0] != '\0')
                {
                    tex_node.SetEmbeddedFormat(ai_tex->achFormatHint);
                }
            }

            return tex_guid;
        });
    }

    // 외부 텍스처 파일 (모델 기준 상대 경로)
    return external_tex_guids.Entry(tex_path_sv).OrInsertWith([&] -> Guid
    {
        const String tex_name = String::Format("Texture_{}_{}", mat_name, slot_name);
        const Guid tex_guid = io_ctx.AllocateSubAssetGuid(tex_name);

        PipelineTextureNode& tex_node = out_container.CreateNode<PipelineTextureNode>(tex_guid);
        tex_node.SetDisplayName(tex_name);
        tex_node.SetSourceFile(model_dir / tex_path_sv);
        tex_node.SetSRGB(srgb);

        return tex_guid;
    });
}

/**
 * aiScene의 모든 머티리얼/텍스처를 glTF PBR Metallic-Roughness 표준에 따라 Pipeline 노드로 변환합니다.
 * @return aiMaterial 인덱스 -> PipelineMaterialInstanceNode UID 배열 (크기 == scene->mNumMaterials)
 */
Array<Guid> ProcessMaterials(
    const aiScene* scene,
    const Path& model_dir,
    ImportContext& io_ctx,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("ProcessMaterials");

    Array<Guid> mat_node_uids;
    if (scene->mNumMaterials == 0)
    {
        return mat_node_uids;
    }

    mat_node_uids.Resize(scene->mNumMaterials, Guid::None);

    // 텍스처 중복 등록 방지 맵
    HashMap<uint32, Guid> embedded_tex_guids;
    HashMap<String, Guid> external_tex_guids;

    for (uint32 mat_idx = 0; mat_idx < scene->mNumMaterials; ++mat_idx)
    {
        const aiMaterial* ai_mat = scene->mMaterials[mat_idx];

        aiString ai_mat_name;
        ai_mat->Get(AI_MATKEY_NAME, ai_mat_name);
        const String mat_name = (ai_mat_name.length > 0)
            ? String(ai_mat_name.C_Str())
            : String::Format("{}", mat_idx);

        const String mat_node_name = String::Format("Material_{}", mat_name);
        const Guid mat_guid = io_ctx.AllocateSubAssetGuid(mat_node_name);
        PipelineMaterialInstanceNode& mat_node = out_container.CreateNode<PipelineMaterialInstanceNode>(mat_guid);
        mat_node.SetDisplayName(mat_node_name);
        mat_node_uids[mat_idx] = mat_guid;

        // --- 스칼라 파라미터 추출 ---
        // base_color_factor: glTF AI_MATKEY_BASE_COLOR 우선, 없으면 FBX AI_MATKEY_COLOR_DIFFUSE
        aiColor4D base_color;
        if (
            ai_mat->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS
            || ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, base_color) == AI_SUCCESS
        )
        {
            mat_node.param_overrides.Insert("base_color_factor", { base_color.r, base_color.g, base_color.b, base_color.a });
        }

        float metallic = 0.0f;
        if (ai_mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
        {
            mat_node.param_overrides.Insert("metallic_factor", { metallic, 0.0f, 0.0f, 0.0f });
        }

        float roughness = 1.0f;
        if (ai_mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
        {
            mat_node.param_overrides.Insert("roughness_factor", { roughness, 0.0f, 0.0f, 0.0f });
        }

        aiColor3D emissive{ 0.0f, 0.0f, 0.0f };
        if (ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
        {
            mat_node.param_overrides.Insert("emissive_factor", { emissive.r, emissive.g, emissive.b, 0.0f });
        }

        // alpha_cutoff: MASK 모드의 투명도 임계값
        float alpha_cutoff = 0.5f;
        if (ai_mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alpha_cutoff) == AI_SUCCESS)
        {
            mat_node.param_overrides.Insert("alpha_cutoff", { alpha_cutoff, 0.0f, 0.0f, 0.0f });
        }

        // alpha mode: MASK이면 셰이더 Flags bit0(alpha test)를 활성화
        aiString alpha_mode;
        if (
            ai_mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alpha_mode) == AI_SUCCESS
            && StringView{ alpha_mode.C_Str() } == "MASK"
        )
        {
            constexpr uint32 flags = std::to_underlying(EMaterialFlag::AlphaTest);
            mat_node.param_overrides.Insert("flags", { std::bit_cast<float>(flags), 0.0f, 0.0f, 0.0f });
        }

        // --- 텍스처 추출 (glTF PBR 5슬롯) ---
        // 각 슬롯은 glTF 우선 -> FBX 호환 순으로 시도
        auto try_extract = [&](StringView slot_name, bool srgb, ArrayView<const aiTextureType> types) -> Guid
        {
            for (const aiTextureType type : types)
            {
                if (const Guid guid = ExtractTexture(ai_mat, type, slot_name, mat_name, srgb, model_dir, scene, embedded_tex_guids, external_tex_guids, io_ctx, out_container))
                {
                    return guid;
                }
            }
            return Guid::None;
        };

        // BaseColor: sRGB
        if (const Guid guid = try_extract("BaseColor", true, { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE }))
        {
            mat_node.texture_node_refs.Insert("BaseColor", guid);
        }

        // MetallicRoughness: linear, G=roughness / B=metallic (glTF 패킹)
        // TODO: FBX처럼 Metalness와 Roughness가 별도 텍스처인 경우, PipelineProcessor에서 하나로 합치는 작업이 필요함
        if (const Guid guid = try_extract("MetallicRoughness", false, { aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS }))
        {
            mat_node.texture_node_refs.Insert("MetallicRoughness", guid);
        }

        // Normal: linear, 접선공간
        if (const Guid guid = try_extract("Normal", false, { aiTextureType_NORMALS, aiTextureType_HEIGHT }))
        {
            mat_node.texture_node_refs.Insert("Normal", guid);
        }

        // Occlusion: linear, R채널
        if (const Guid guid = try_extract("Occlusion", false, { aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP }))
        {
            mat_node.texture_node_refs.Insert("Occlusion", guid);
        }

        // Emissive: sRGB
        if (const Guid guid = try_extract("Emissive", true, { aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR }))
        {
            mat_node.texture_node_refs.Insert("Emissive", guid);
        }
    }

    return mat_node_uids;
}
} // namespace

namespace se::editor
{
ArrayView<const StringView> AssimpTranslator::GetSupportedExtensions() const
{
    static constexpr FixedArray supported_extensions = MakeFixedArray<StringView>(
        ".obj", ".fbx", ".gltf", ".glb", ".blend", ".vrm", ".pmx", ".pmd"
    );
    return supported_extensions;
}

void AssimpTranslator::Translate(
    const Path& file_path,
    const ImportProfile& import_profile,
    ImportContext& io_ctx,
    PipelineNodeContainer& out_container
)
{
    Assimp::Importer importer;

    // 설정 불러오기 (없으면 기본값)
    const MeshImportSettings mesh_settings = import_profile.GetOrDefault<MeshImportSettings>();

    // 기본 플래그 설정
    uint32 flags =
        aiProcess_Triangulate             // 모든 면을 삼각형으로 변환
        | aiProcess_GenSmoothNormals      // 부드러운 노멀 생성
        | aiProcess_CalcTangentSpace      // 노멀 매핑을 위한 탄젠트 계산
        | aiProcess_JoinIdenticalVertices // 중복 정점 제거 (최적화)
        | aiProcess_SortByPType           // 점/선 제거하고 다각형만 남김
        | aiProcess_LimitBoneWeights      // 최대 4개의 Bone Weight 사용
        | aiProcess_FlipUVs;              // UV 좌표를 위아래로 뒤집음 (SDL3 GPU의 Top-Left 원점으로 통일)

    const bool use_ptv = mesh_settings.combine_meshes || mesh_settings.apply_transform;
    if (use_ptv)
    {
        // 노드 계층구조를 무시하고 모든 변환을 정점에 미리 적용 (StaticMesh 전용)
        flags |= aiProcess_PreTransformVertices;

        // 인접한 메쉬들을 병합하고 불필요한 노드를 제거하여 드로우 콜(Draw Call) 수를 최적화
        flags |= aiProcess_OptimizeMeshes;

        // Y-up RH -> Z-up RH 변환 행렬을 root transform으로 주입
        // PTV가 position, normal, tangent, bitangent 등 모든 attribute를 자동 변환
        importer.SetPropertyInteger(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, 1);
        importer.SetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, YUP_TO_ZUP_COL);
    }

    // Global Scale 적용 (Assimp Property)
    if (math::Abs(mesh_settings.global_scale - 1.0f) > math::KINDA_SMALL_NUMBER)
    {
        // TODO: 여기 지우고, PipelineProcessor에서 하는걸로 수정
    }

    // 파일 로드
    const aiScene* const scene = [&]
    {
        ZoneScopedN("Assimp::ReadFile"); // NOLINT(*-lambda-function-name)
        return importer.ReadFile(file_path.CStr(), flags);
    }();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        ConsoleLog(ELogLevel::Error, "Assimp Load Failed: {}, path: {}", importer.GetErrorString(), file_path);
        return;
    }

    if (mesh_settings.combine_meshes)
    {
        // 파일명을 Mesh의 이름으로 사용
        const String filename = file_path.FileStem().ValueOr("Unnamed");
        ProcessMergedMesh(scene, filename, io_ctx, out_container);
    }
    else
    {
        // PTV 활성: 이미 Z-up (convert=false) | PTV 비활성: 수동 변환 (convert=true)
        ProcessNodeIterative(scene->mRootNode, scene, !use_ptv, io_ctx, out_container);
    }

    // 머티리얼/텍스처 노드 생성 후 각 StaticMeshPipelineNode에 연결
    const Path model3d_dir = file_path.Parent().ValueOrDefault();
    const Array<Guid> mat_node_uids = ProcessMaterials(scene, model3d_dir, io_ctx, out_container);

    if (!mat_node_uids.IsEmpty())
    {
        for (const auto& node_ptr : out_container.GetAllNodes() | std::views::values)
        {
            if (StaticMeshPipelineNode* mesh_node = Cast<StaticMeshPipelineNode>(node_ptr.get()))
            {
                mesh_node->material_node_uids = mat_node_uids;
            }
        }
    }
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
