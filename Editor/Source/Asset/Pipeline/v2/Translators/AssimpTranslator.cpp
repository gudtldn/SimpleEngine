// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/v2/Translators/AssimpTranslator.h"

#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "assimp/config.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "tracy/Tracy.hpp"


namespace
{
using namespace se;
using namespace se::editor;
using namespace se::editor::v2;

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

float ComputeTangentSign(const Vector3f& normal, const Vector3f& tangent, const Vector3f& bitangent)
{
    return normal.Cross(tangent).Dot(bitangent) >= 0.0f ? 1.0f : -1.0f;
}

Matrix4x4f ConvertNodeTransform(const aiMatrix4x4& ai_mat)
{
    const Matrix4x4f t_yup_row = {
        ai_mat.a1, ai_mat.b1, ai_mat.c1, ai_mat.d1,
        ai_mat.a2, ai_mat.b2, ai_mat.c2, ai_mat.d2,
        ai_mat.a3, ai_mat.b3, ai_mat.c3, ai_mat.d3,
        ai_mat.a4, ai_mat.b4, ai_mat.c4, ai_mat.d4
    };

    return ZUP_TO_YUP_ROW * t_yup_row * YUP_TO_ZUP_ROW;
}

void ExtractVertices(const aiMesh* mesh, bool convert_to_zup, Array<StaticVertex>& out_vertices)
{
    out_vertices.Reserve(out_vertices.Len() + mesh->mNumVertices);

    auto calc_transform = [](const Vector3f& v, float w)
    {
        return Vector3f{ Vector4f{ v, w } * YUP_TO_ZUP_ROW };
    };

    for (uint32 i = 0; i < mesh->mNumVertices; ++i)
    {
        StaticVertex vertex;

        Vector3f pos = ToVec3f(mesh->mVertices[i]);
        vertex.position = convert_to_zup ? calc_transform(pos, 1.0f) : pos;

        if (mesh->HasNormals())
        {
            Vector3f nrm = ToVec3f(mesh->mNormals[i]);
            vertex.normal = convert_to_zup ? calc_transform(nrm, 0.0f) : nrm;
        }

        if (mesh->HasTangentsAndBitangents())
        {
            Vector3f t = ToVec3f(mesh->mTangents[i]);
            Vector3f b = ToVec3f(mesh->mBitangents[i]);

            if (convert_to_zup)
            {
                t = calc_transform(t, 0.0f);
                b = calc_transform(b, 0.0f);
            }

            vertex.tangent = Vector4f{ t, ComputeTangentSign(vertex.normal, t, b) };
        }

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

    uint32 total_vertices = 0;
    uint32 total_indices = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        total_vertices += scene->mMeshes[i]->mNumVertices;
        total_indices += scene->mMeshes[i]->mNumFaces * 3;
    }
    pipeline_node.vertices.Reserve(total_vertices);
    pipeline_node.indices.Reserve(total_indices);

    uint32 vertex_offset = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        ExtractVertices(scene->mMeshes[i], false, pipeline_node.vertices);
        ExtractIndices(scene->mMeshes[i], vertex_offset, pipeline_node.indices);
        vertex_offset += scene->mMeshes[i]->mNumVertices;
    }

    if (scene->mNumMeshes > 0)
    {
        pipeline_node.material_index = scene->mMeshes[0]->mMaterialIndex;
    }
}

void ProcessNodeIterative(
    const aiNode* root_node,
    const aiScene* scene,
    bool convert_to_zup,
    ImportContext& io_ctx,
    PipelineNodeContainer& out_container
)
{
    Array<const aiNode*> stack;
    stack.Push(root_node);

    while (Optional node_opt = stack.Pop())
    {
        const aiNode* node = *node_opt;

        for (uint32 i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            const String node_name = String::Format("{}_{}", node->mName.C_Str(), mesh->mName.C_Str());

            const Guid guid = io_ctx.AllocateSubAssetGuid(node_name);
            StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>(guid);
            pipeline_node.SetDisplayName(node_name);
            pipeline_node.material_index = mesh->mMaterialIndex;

            ExtractVertices(mesh, convert_to_zup, pipeline_node.vertices);
            ExtractIndices(mesh, 0, pipeline_node.indices);

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
} // namespace

namespace se::editor::v2
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

    const MeshImportSettings mesh_settings = import_profile.GetOrDefault<MeshImportSettings>();

    uint32 flags =
        aiProcess_Triangulate
        | aiProcess_GenSmoothNormals
        | aiProcess_CalcTangentSpace
        | aiProcess_JoinIdenticalVertices
        | aiProcess_SortByPType
        | aiProcess_LimitBoneWeights;

    const bool use_ptv = mesh_settings.combine_meshes || mesh_settings.apply_transform;
    if (use_ptv)
    {
        flags |= aiProcess_PreTransformVertices;
        flags |= aiProcess_OptimizeMeshes;

        importer.SetPropertyInteger(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, 1);
        importer.SetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, YUP_TO_ZUP_COL);
    }

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
        const String filename = file_path.FileStem().ValueOr("Unnamed");
        ProcessMergedMesh(scene, filename, io_ctx, out_container);
    }
    else
    {
        ProcessNodeIterative(scene->mRootNode, scene, !use_ptv, io_ctx, out_container);
    }
}
} // namespace se::editor::v2
// NOLINTEND(*-reserved-identifier)
