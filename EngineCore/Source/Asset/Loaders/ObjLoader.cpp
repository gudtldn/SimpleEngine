#include "SimpleEngine/Asset/Loaders/ObjLoader.h"

#include "Asset/Types/MeshTypes.h"
#include "Reflection/Reflect.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"


namespace se::asset
{
SE_BEGIN_REFLECT(ObjLoader)
SE_END_REFLECT(ObjLoader)

concurrency::Task<std::shared_ptr<IAsset>> ObjLoader::Load(
    const std::filesystem::path& physical_path,
    const IAssetImportSettings* import_settings
)
{
    // TODO: import_settings에 따라서 flag나 Import 설정 변경

    using uint = unsigned int;
    Assimp::Importer importer;

    // StaticMesh용 최적화 플래그 조합
    constexpr uint flags =
        aiProcess_Triangulate             // 삼각형으로 변환
        | aiProcess_GenSmoothNormals      // 부드러운 노멀 생성
        | aiProcess_CalcTangentSpace      // 노멀 매핑을 위한 탄젠트 계산
        | aiProcess_JoinIdenticalVertices // 중복 정점 제거 (최적화)
        | aiProcess_FlipUVs               // 텍스처 좌표 Y축 반전 (엔진 좌표계에 따라 조절)
        | aiProcess_PreTransformVertices  // 노드 계층구조를 무시하고 모든 변환을 정점에 미리 적용 (StaticMesh 전용)
        | aiProcess_SortByPType;          // 점/선 제거하고 다각형만 남김

    // 파일 로딩
    const aiScene* scene = importer.ReadFile(physical_path.string(), flags);

    // 에러 체크
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||!scene->mRootNode)
    {
        ConsoleLog(ELogLevel::Error, "[ObjLoader] Assimp Load Failed: {}", importer.GetErrorString());
        co_return nullptr;
    }

    // 메모리 할당 최적화를 위한 전체 크기 계산
    uint32 total_vertices = 0;
    uint32 total_indices = 0;
    for (uint i = 0; i < scene->mNumMeshes; ++i)
    {
        total_vertices += scene->mMeshes[i]->mNumVertices;
        total_indices += scene->mMeshes[i]->mNumFaces * 3; // Triangulate 했으므로 * 3
    }

    auto static_mesh = std::make_shared<StaticMesh>();
    static_mesh->vertices.Reserve(total_vertices);
    static_mesh->indices.Reserve(total_indices);
    static_mesh->sections.Reserve(scene->mNumMeshes);

    // 메쉬 데이터 병합 (Merge)
    uint32 vertex_offset = 0;
    uint32 index_offset = 0;

    // PreTransformVertices 플래그 덕분에 노드 순회 없이 mMeshes 배열만 순회하면 됨
    for (uint i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* ai_mesh = scene->mMeshes[i];

        // --- Vertex Processing ---
        for (uint vert_idx = 0; vert_idx < ai_mesh->mNumVertices; ++vert_idx)
        {
            Vertex vertex;

            // Position
            vertex.position = {
                ai_mesh->mVertices[vert_idx].x,
                ai_mesh->mVertices[vert_idx].y,
                ai_mesh->mVertices[vert_idx].z
            };

            // Normal
            if (ai_mesh->HasNormals())
            {
                vertex.normal = {
                    ai_mesh->mNormals[vert_idx].x,
                    ai_mesh->mNormals[vert_idx].y,
                    ai_mesh->mNormals[vert_idx].z
                };
            }

            // Tangent (Normal Map용)
            if (ai_mesh->HasTangentsAndBitangents())
            {
                vertex.tangent = {
                    ai_mesh->mTangents[vert_idx].x,
                    ai_mesh->mTangents[vert_idx].y,
                    ai_mesh->mTangents[vert_idx].z,
                    1.0f
                };
            }

            // TexCoord (0번 채널만 사용)
            if (ai_mesh->HasTextureCoords(0))
            {
                vertex.tex_coord = {
                    ai_mesh->mTextureCoords[vert_idx]->x,
                    ai_mesh->mTextureCoords[vert_idx]->y
                };
            }
            else
            {
                vertex.tex_coord = { 0.0f, 0.0f };
            }

            static_mesh->vertices.Push(vertex);
        }

        // --- Index Processing ---
        uint32 current_mesh_indices_count = 0;
        for (uint face_idx = 0; face_idx < ai_mesh->mNumFaces; ++face_idx)
        {
            const aiFace& face = ai_mesh->mFaces[face_idx];
            for (uint j = 0; j < face.mNumIndices; j++)
            {
                // 글로벌 인덱스 = 로컬 인덱스 + 현재까지의 정점 오프셋
                static_mesh->indices.Push(face.mIndices[j] + vertex_offset);
                current_mesh_indices_count++;
            }
        }

        // --- Section Info ---
        MeshSection section;
        section.material_index = ai_mesh->mMaterialIndex;
        section.index_start = index_offset;
        section.index_count = current_mesh_indices_count;

        static_mesh->sections.Push(section);

        // 오프셋 갱신
        vertex_offset += ai_mesh->mNumVertices;
        index_offset += current_mesh_indices_count;
    }

    co_return static_mesh;
}
} // namespace se::asset
