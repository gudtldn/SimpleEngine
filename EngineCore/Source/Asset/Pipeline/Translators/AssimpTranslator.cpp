#include "Asset/Pipeline/Translators/AssimpTranslator.h"

#include "Asset/ImportSettings/MeshImportSettings.h"
#include "Asset/Pipeline/Nodes/StaticMeshPipelineNode.h"
#include "Utility/StringUtils.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "tracy/Tracy.hpp"


namespace
{
using namespace se;
using namespace se::asset;

void ProcessMergedMesh(
    const aiScene* scene,
    const String& mesh_name,
    PipelineNodeContainer& out_container
)
{
    ZoneScopedN("ProcessMergedMesh");

    StaticMeshPipelineNode& pipeline_node = out_container.CreateNode<StaticMeshPipelineNode>();
    pipeline_node.SetDisplayName(mesh_name);

    // 전체 크기 계산
    uint32 total_vertices = 0;
    uint32 total_indices = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        total_vertices += scene->mMeshes[i]->mNumVertices;
        total_indices += scene->mMeshes[i]->mNumFaces * 3;
    }
    pipeline_node.vertices.Reserve(total_vertices);
    pipeline_node.indices.Reserve(total_indices);

    // 병합 루프
    uint32 vertex_offset = 0;
    uint32 index_offset = 0;
    for (uint32 i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[i];

        // Vertices 변환
        for (uint32 v = 0; v < mesh->mNumVertices; ++v)
        {
            gfx::Vertex vertex;

            // Position
            vertex.position = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };

            // Normal
            if (mesh->HasNormals())
            {
                vertex.normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
            }

            // UV
            if (mesh->HasTextureCoords(0))
            {
                vertex.tex_coord = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
            }

            // Tangent
            if (mesh->HasTangentsAndBitangents())
            {
                vertex.tangent = { mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z, 1.0f };
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

        // Section(Submesh) 설정
        gfx::MeshSection section;
        section.index_start = index_offset;
        section.index_count = mesh->mNumFaces * 3;
        section.material_index = mesh->mMaterialIndex;

        pipeline_node.sections.Push(section);

        vertex_offset += mesh->mNumVertices;
        index_offset += section.index_count;
    }

    // TODO: 여기서 머티리얼 의존성(Material Node UID)을 추가
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
        gfx::Vertex vertex;

        vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        // Normal
        if (mesh->HasNormals())
        {
            vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            vertex.tex_coord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }

        // Tangent
        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f };
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

    // Section(Submesh) 설정
    gfx::MeshSection section;
    section.index_start = 0;
    section.index_count = static_cast<uint32>(pipeline_node.indices.Len());
    section.material_index = mesh->mMaterialIndex;

    pipeline_node.sections.Push(section);

    // TODO: 여기서 머티리얼 의존성(Material Node UID)을 추가
}

void ProcessNodeRecursive(const aiNode* node, const aiScene* scene, PipelineNodeContainer& out_container)
{
    // 현재 노드에 있는 모든 메쉬 처리
    for (uint32 i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessSingleMesh(mesh, scene, node->mName.C_Str(), out_container);
    }

    // 자식 노드 순회
    for (uint32 i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNodeRecursive(node->mChildren[i], scene, out_container);
    }
}
}  // namespace

namespace se::asset
{
bool AssimpTranslator::CanTranslate(const String& file_extension) const
{
    static HashSet<String> supported_extensions = { ".obj", ".fbx", ".gltf", ".glb" };
    return supported_extensions.Contains(file_extension);
}

void AssimpTranslator::Translate(
    const std::filesystem::path& file_path,
    const ImportConfig& import_config,
    PipelineNodeContainer& out_container
)
{
    Assimp::Importer importer;

    // 설정 불러오기 (없으면 기본값)
    const MeshImportSettings mesh_settings = import_config.GetOrDefault<MeshImportSettings>();

    // 기본 플래그 설정
    uint32 flags =
        aiProcess_Triangulate             // 모든 면을 삼각형으로 변환
        | aiProcess_GenSmoothNormals      // 부드러운 노멀 생성
        | aiProcess_CalcTangentSpace      // 노멀 매핑을 위한 탄젠트 계산
        | aiProcess_JoinIdenticalVertices // 중복 정점 제거 (최적화)
        | aiProcess_SortByPType;          // 점/선 제거하고 다각형만 남김

    if (mesh_settings.combine_meshes || mesh_settings.apply_transform)
    {
        // 노드 계층구조를 무시하고 모든 변환을 정점에 미리 적용 (StaticMesh 전용)
        flags |= aiProcess_PreTransformVertices;
    }

    // Global Scale 적용 (Assimp Property)
    if (math::Abs(mesh_settings.global_scale - 1.0f) > math::KINDA_SMALL_NUMBER)
    {
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, mesh_settings.global_scale);
    }

    // 파일 로드
    const String utf8_path = utility::ToString(file_path.generic_u8string());
    const aiScene* scene = [&]
    {
        ZoneScopedN("Assimp::ReadFile");
        return importer.ReadFile(utf8_path.CStr(), flags);
    }();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        ConsoleLog(ELogLevel::Error, "Assimp Load Failed: {}, path: {}", importer.GetErrorString(), utf8_path);
        return;
    }

    if (mesh_settings.combine_meshes)
    {
        // 파일명을 메쉬 이름으로 사용
        const String filename = utility::ToString(file_path.filename().stem().u8string());
        ProcessMergedMesh(scene, filename, out_container);
    }
    else
    {
        ProcessNodeRecursive(scene->mRootNode, scene, out_container);
    }
}
}  // namespace se::asset
