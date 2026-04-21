// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/Translators/AssimpTranslator.h"

#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "tracy/Tracy.hpp"


namespace
{
using namespace se;
using namespace se::editor;

// Assimp Y-up RH -> Engine Z-up RH: (x, y, z) -> (x, -z, y)
struct CoordConvert
{
    static Vector3f Position(const aiVector3D& v) { return { v.x, -v.z, v.y }; }
    static Vector3f Normal(const aiVector3D& v)   { return { v.x, -v.z, v.y }; }
    static Vector4f Tangent(const aiVector3D& v)  { return { v.x, -v.z, v.y, 1.0f }; }
};

/**
 * 씬의 모든 primitive를 하나의 StaticMeshPipelineNode로 병합합니다.
 * combine_meshes=true일 때 1개의 StaticMesh 에셋(1 draw call)을 생성합니다.
 *
 * 또한 1개의 material만 지원하며, primitive가 여러 material을 가지더라도 첫 번째 primitive의 material을 사용합니다.
 * 여러 material이 필요하면 combine_meshes=false를 사용하세요.
 */
void ProcessMergedMesh(
    const aiScene* scene,
    const String& mesh_name,
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

    // 모든 primitive를 하나의 버퍼에 병합
    uint32 vertex_offset = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];

        // Vertices 변환
        for (uint32 v = 0; v < mesh->mNumVertices; ++v)
        {
            graphics::Vertex vertex;

            // Position (Y-up -> Z-up)
            vertex.position = CoordConvert::Position(mesh->mVertices[v]);

            // Normal
            if (mesh->HasNormals())
            {
                vertex.normal = CoordConvert::Normal(mesh->mNormals[v]);
            }

            // UV
            if (mesh->HasTextureCoords(0))
            {
                vertex.tex_coord = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
            }

            // Tangent
            if (mesh->HasTangentsAndBitangents())
            {
                vertex.tangent = CoordConvert::Tangent(mesh->mTangents[v]);
            }

            pipeline_node.vertices.Push(vertex);
        }

        // Indices 변환
        for (uint32 f = 0; f < mesh->mNumFaces; ++f)
        {
            const aiFace& face = mesh->mFaces[f];
            for (uint32 j = 0; j < face.mNumIndices; ++j)
            {
                pipeline_node.indices.Push(face.mIndices[j] + vertex_offset);
            }
        }

        vertex_offset += mesh->mNumVertices;
    }

    if (scene->mNumMeshes > 0)
    {
        pipeline_node.material_index = scene->mMeshes[0]->mMaterialIndex;
    }
}

void ProcessSingleMesh(
    const aiMesh* mesh,
    [[maybe_unused]] const aiScene* scene,
    const char* node_name,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("ProcessSingleMesh");

    StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>();

    // 노드 이름 설정
    pipeline_node.SetDisplayName(String::Format("{}_{}", node_name, mesh->mName.C_Str()));

    // Vertices 변환
    pipeline_node.vertices.Reserve(mesh->mNumVertices);
    for (uint32 i = 0; i < mesh->mNumVertices; ++i)
    {
        graphics::Vertex vertex;

        vertex.position = CoordConvert::Position(mesh->mVertices[i]);

        // Normal
        if (mesh->HasNormals())
        {
            vertex.normal = CoordConvert::Normal(mesh->mNormals[i]);
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            vertex.tex_coord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }

        // Tangent
        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent = CoordConvert::Tangent(mesh->mTangents[i]);
        }

        pipeline_node.vertices.Push(vertex);
    }

    // Indices 변환
    pipeline_node.indices.Reserve(mesh->mNumFaces * 3ULL);
    for (uint32 i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; ++j)
        {
            pipeline_node.indices.Push(face.mIndices[j]);
        }
    }

    pipeline_node.material_index = mesh->mMaterialIndex;
}

void ProcessNodeIterative(const aiNode* root_node, const aiScene* scene, PipelineNodeContainer& out_container)
{
    Array<const aiNode*> stack;
    stack.Push(root_node);

    while (Optional node_opt = stack.Pop())
    {
        const aiNode* node = *node_opt;

        // 메쉬 처리
        for (uint32 i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessSingleMesh(mesh, scene, node->mName.C_Str(), out_container);
        }

        // 자식 노드들을 스택에 추가
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
    // Y-up -> Z-up 변환은 CoordConvert에서 처리
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

    if (mesh_settings.combine_meshes || mesh_settings.apply_transform)
    {
        // 노드 계층구조를 무시하고 모든 변환을 정점에 미리 적용 (StaticMesh 전용)
        flags |= aiProcess_PreTransformVertices;

        // 인접한 메쉬들을 병합하고 불필요한 노드를 제거하여 드로우 콜(Draw Call) 수를 최적화
        flags |= aiProcess_OptimizeMeshes;
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
        ProcessMergedMesh(scene, filename, out_container);
    }
    else
    {
        ProcessNodeIterative(scene->mRootNode, scene, out_container);
    }
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
