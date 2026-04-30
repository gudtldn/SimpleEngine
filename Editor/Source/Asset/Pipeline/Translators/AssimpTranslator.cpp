// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/Translators/AssimpTranslator.h"

#include "Asset/Pipeline/Translators/AssimpMaterialExtractor.h"
#include "Asset/Pipeline/Translators/AssimpTextureExtractor.h"

#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "assimp/config.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "tracy/Tracy.hpp"


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
void ExtractVertices(const aiMesh* mesh, bool convert_to_zup, Array<graphics::StaticVertex>& out_vertices)
{
    out_vertices.Reserve(out_vertices.Len() + mesh->mNumVertices);

    // 변환 행렬 적용 헬퍼: v_zup = Vector4f(v_yup, w) * YUP_TO_ZUP_ROW
    auto calc_transform = [](const Vector3f& v, float w)
    {
        return Vector3f{ Vector4f{ v, w } * YUP_TO_ZUP_ROW };
    };

    for (uint32 i = 0; i < mesh->mNumVertices; ++i)
    {
        graphics::StaticVertex vertex;

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
 * 각 aiMesh는 SubMeshSection으로 분리되어 서브 메시 당 개별 머티리얼을 지원합니다.
 */
void ProcessMergedMesh(
    const aiScene* scene,
    const String& mesh_name,
    const AssimpMaterialExtractor::MaterialExtractionResult& material_result,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("ProcessMergedMesh");

    StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>();
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

    // 각 primitive를 SubMeshSection으로 기록 (로컬 인덱스 + SDL base_vertex 방식)
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];

        StaticMeshPipelineNode::SubMeshSection section;
        section.index_offset  = static_cast<uint32>(pipeline_node.indices.Len());
        section.vertex_offset = static_cast<uint32>(pipeline_node.vertices.Len());

        ExtractVertices(mesh, false, pipeline_node.vertices);
        ExtractIndices(mesh, 0, pipeline_node.indices);  // 로컬 인덱스; base_vertex는 런타임에 적용

        section.index_count = static_cast<uint32>(pipeline_node.indices.Len()) - section.index_offset;

        if (mesh->mMaterialIndex < material_result.material_index_to_guid.Len())
        {
            section.material_node_uid = material_result.material_index_to_guid[mesh->mMaterialIndex];
        }

        pipeline_node.sections.Push(section);
    }
}

/**
 * 노드 계층을 순회하며 각 aiNode를 개별 PipelineNode로 생성합니다.
 * aiNode 내 여러 aiMesh는 SubMeshSection으로 분리되어 서브 메시 당 개별 머티리얼을 지원합니다.
 *
 * convert_to_zup=true (비-PTV 경로): Y-up -> Z-up 변환을 수동 적용하고 노드 transform을 보존
 * convert_to_zup=false (PTV 경로): PTV가 이미 변환 완료했으므로 직접 읽기
 */
void ProcessNodeIterative(
    const aiNode* root_node,
    const aiScene* scene,
    bool convert_to_zup,
    const AssimpMaterialExtractor::MaterialExtractionResult& material_result,
    PipelineNodeContainer& out_container
)
{
    Array<const aiNode*> stack;
    stack.Push(root_node);

    while (Optional node_opt = stack.Pop())
    {
        const aiNode* node = *node_opt;

        if (node->mNumMeshes > 0)
        {
            StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>();
            pipeline_node.SetDisplayName(node->mName.C_Str());

            // 비-PTV 경로: 노드의 로컬 트랜스폼을 Z-up으로 변환하여 보존
            if (convert_to_zup)
            {
                pipeline_node.local_transform = ConvertNodeTransform(node->mTransformation);
            }

            // aiNode 내 각 aiMesh를 SubMeshSection으로 기록
            for (uint32 i = 0; i < node->mNumMeshes; ++i)
            {
                const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

                StaticMeshPipelineNode::SubMeshSection section;
                section.index_offset  = static_cast<uint32>(pipeline_node.indices.Len());
                section.vertex_offset = static_cast<uint32>(pipeline_node.vertices.Len());

                ExtractVertices(mesh, convert_to_zup, pipeline_node.vertices);
                ExtractIndices(mesh, 0, pipeline_node.indices);  // 로컬 인덱스; base_vertex는 런타임에 적용

                section.index_count = static_cast<uint32>(pipeline_node.indices.Len()) - section.index_offset;

                if (mesh->mMaterialIndex < material_result.material_index_to_guid.Len())
                {
                    section.material_node_uid = material_result.material_index_to_guid[mesh->mMaterialIndex];
                }

                pipeline_node.sections.Push(section);
            }
        }

        for (uint32 i = 0; i < node->mNumChildren; ++i)
        {
            stack.Push(node->mChildren[i]);
        }
    }
}
} // namespace

namespace se::editor
{
SE_BEGIN_REFLECT(AssimpTranslator, meta::Internal)
    SE_REFLECT_INTERFACE(IPipelineTranslator)
SE_END_REFLECT(AssimpTranslator)

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
        | aiProcess_LimitBoneWeights;     // 최대 4개의 Bone Weight 사용

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

    // 텍스처 노드 생성
    const TextureExtractionResult texture_result = AssimpTextureExtractor::ExtractTexturesFromScene(scene, file_path, out_container);

    // 머티리얼 노드 생성 (TextureExtractionResult를 사용하여 텍스처 GUID 참조)
    const AssimpMaterialExtractor::MaterialExtractionResult material_result =
        AssimpMaterialExtractor::ExtractMaterialsFromScene(file_path, scene, texture_result, out_container);

    if (mesh_settings.combine_meshes)
    {
        // 파일명을 Mesh의 이름으로 사용
        const String filename = file_path.FileStem().ValueOr("Unnamed");
        ProcessMergedMesh(scene, filename, material_result, out_container);
    }
    else
    {
        // PTV 활성: 이미 Z-up (convert=false) | PTV 비활성: 수동 변환 (convert=true)
        ProcessNodeIterative(scene->mRootNode, scene, !use_ptv, material_result, out_container);
    }
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
