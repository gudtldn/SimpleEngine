#include "Asset/Pipeline/Translators/AssimpTranslator.h"
#include "Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "Utility/StringUtils.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"


namespace
{
using namespace se;
using namespace se::asset;

void ProcessMesh(
    const aiMesh* mesh,
    [[maybe_unused]] const aiScene* scene,
    const char* node_name,
    PipelineNodeContainer& out_container
)
{
    StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>();

    // 노드 이름 설정
    pipeline_node.SetDisplayName(String::Format("{}_{}", node_name, mesh->mName.C_Str()));

    // Vertices 변환
    pipeline_node.vertices.Reserve(mesh->mNumVertices);
    for (uint32 i = 0; i < mesh->mNumVertices; ++i)
    {
        gfx::Vertex vertex;

        // Position
        vertex.position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z,
        };

        // Normal
        if (mesh->HasNormals())
        {
            vertex.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z,
            };
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            vertex.tex_coord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        // Tangent
        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z,
                1.0f
            };
        }

        pipeline_node.vertices.Push(vertex);
    }

    // Indices 변환
    pipeline_node.indices.Reserve(mesh->mNumFaces * 3);
    for (uint32 i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (uint32 j = 0; j < face.mNumIndices; ++j)
        {
            pipeline_node.indices.Push(face.mIndices[j]);
        }
    }

    // Section(Submesh) 설정
    gfx::MeshSection section;
    section.index_start = 0;
    section.index_count = static_cast<uint32>(pipeline_node.indices.Len());
    section.material_index = mesh->mMaterialIndex;

    pipeline_node.sections.Push(section);

    // TODO: 여기서 머티리얼 의존성(Material Node UID)을 추가
}

void ProcessNode(const aiNode* node, const aiScene* scene, PipelineNodeContainer& out_container)
{
    // 현재 노드에 있는 모든 메쉬 처리
    for (uint32 i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene, node->mName.C_Str(), out_container);
    }

    // 자식 노드 순회
    for (uint32 i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene, out_container);
    }
}
}

namespace se::asset
{
bool AssimpTranslator::CanTranslate(const std::filesystem::path& file_extension) const
{
    const String ext = utility::ToString(file_extension.u8string()).ToLower();

    static HashSet<String> supported_extensions = { ".obj", ".fbx", ".gltf", ".glb" };
    return supported_extensions.Contains(ext);
}

void AssimpTranslator::Translate(const std::filesystem::path& file_path, PipelineNodeContainer& out_container)
{
    Assimp::Importer importer;

    // 플래그 설정
    constexpr uint32 flags =
        aiProcess_Triangulate             // 모든 면을 삼각형으로 변환
        | aiProcess_GenSmoothNormals      // 부드러운 노멀 생성
        | aiProcess_CalcTangentSpace      // 노멀 매핑을 위한 탄젠트 계산
        | aiProcess_JoinIdenticalVertices // 중복 정점 제거 (최적화)
        // | aiProcess_PreTransformVertices  // 노드 계층구조를 무시하고 모든 변환을 정점에 미리 적용 (StaticMesh 전용)
        | aiProcess_SortByPType;          // 점/선 제거하고 다각형만 남김

    // 파일 로딩
    const String utf8_path = utility::ToString(file_path.u8string());
    const aiScene* scene = importer.ReadFile(utf8_path.CStr(), flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        ConsoleLog(ELogLevel::Error, "Assimp Load Failed: {}, path: {}", importer.GetErrorString(), utf8_path);
        return;
    }

    // 재귀적으로 노드를 순회하며 메쉬 처리
    ProcessNode(scene->mRootNode, scene, out_container);
}
}
