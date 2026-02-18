#pragma once
#include <shared_mutex>

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
/**
 * 파일 내 개별 Asset 정보를 나타내는 구조체
 */
struct AssetEntry
{
    AssetId id;
    TypeId type;
};

/**
 * Asset의 Path/Id/Type/Meta 매핑을 관리하는 중앙 레지스트리
 *
 * Editor가 .meta 파일을 파싱하여 이 Registry에 데이터를 주입합니다.
 * Core(Runtime)는 주입된 매핑을 통해 Asset을 조회합니다.
 *
 * SaveToFile / LoadFromFile을 통해 바이너리로 저장할 수 있으며,
 * 에디터 재시작 시 .meta 재파싱 없이도 Registry를 복원할 수 있습니다.
 */
class SE_CORE_API AssetRegistry
{
public:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;
    AssetRegistry(AssetRegistry&&) = delete;
    AssetRegistry& operator=(AssetRegistry&&) = delete;

public:
    // ---- 데이터 등록 (Editor가 호출) ----

    /** 새로운 Asset을 Registry에 등록합니다. */
    void RegisterAsset(const AssetId& asset_id, const TypeId& asset_type, AssetPath&& asset_path);

    /**
     * Asset에 대한 메타 데이터를 등록합니다.
     * RegisterAsset 이후 또는 동시에 호출합니다.
     *
     * @param asset_id 등록할 Asset의 식별자
     * @param meta 저장할 메타 데이터
     */
    void RegisterMeta(const AssetId& asset_id, AssetMetadata&& meta);

    /**
     * 등록된 Asset을 Registry에서 제거합니다.
     * 모든 내부 맵(path_to_id, id_to_path, id_to_type, file_to_assets, id_to_meta)에서
     * 해당 항목을 삭제합니다.
     *
     * @param asset_id 제거할 Asset의 식별자
     */
    void UnregisterAsset(const AssetId& asset_id);

    /** Registry의 모든 데이터를 초기화합니다. */
    void Clear();

public:
    // ---- 조회 (Core/Editor 공용) ----

    /** AssetPath에 해당하는 AssetId를 반환합니다. */
    [[nodiscard]] Optional<const AssetId&> GetAssetId(const AssetPath& asset_path) const;

    /** AssetId에 해당하는 AssetPath를 반환합니다. */
    [[nodiscard]] Optional<const AssetPath&> GetAssetPath(const AssetId& asset_id) const;

    /** AssetId에 해당하는 TypeId를 반환합니다. */
    [[nodiscard]] Optional<const TypeId&> GetAssetType(const AssetId& asset_id) const;

    /** 파일 내에서 특정 타입의 첫 번째 Asset ID를 찾습니다. */
    [[nodiscard]] Optional<const AssetId&> FindFirstOfType(const Path& file_path, const TypeId& type) const;

    /** 파일에 등록된 모든 Asset 목록을 반환합니다. */
    [[nodiscard]] Optional<const Array<AssetEntry>&> GetAssetsInFile(const Path& file_path) const;

    /** AssetId에 대응하는 메타 데이터를 반환합니다. */
    [[nodiscard]] Optional<const AssetMetadata&> GetMeta(const AssetId& asset_id) const;

    /** Path에 있는 파일을 Import 한걸로 표시합니다. */
    void MarkFileAsImported(const Path& file_path);

    /** Path에 있는 파일이 Import되었는지 확인합니다. */
    [[nodiscard]] bool IsFileImported(const Path& file_path) const;

    /** 현재 등록된 Asset의 총 개수를 반환합니다. */
    [[nodiscard]] uint32 GetAssetCount() const;

public:
    // ---- Registry 상태를 바이너리로 역/직렬화 ----

    /**
     * Registry 전체를 바이너리 파일로 저장합니다.
     * 에디터 종료 또는 프로젝트 저장 시 호출합니다.
     *
     * @param file_path 저장할 바이너리 파일 경로
     * @return 저장 성공 여부
     */
    [[nodiscard]] bool SaveToFile(const Path& file_path) const;

    /**
     * 바이너리 파일에서 Registry를 복원합니다.
     * 에디터 시작 시 .meta 재파싱 없이 빠르게 복원하는 용도입니다.
     *
     * @param file_path 읽어올 바이너리 파일 경로
     * @return 로드 성공 여부
     */
    [[nodiscard]] bool LoadFromFile(const Path& file_path);

private:
    mutable TracySharedLockable(std::shared_mutex, registry_mutex);

    // Import된 파일 추적 (중복 Import 방지)
    HashSet<Path> imported_files;

    // AssetPath <-> AssetId 양방향 매핑
    HashMap<AssetPath, AssetId> path_to_id;
    HashMap<AssetId, AssetPath> id_to_path;

    // AssetId -> TypeId 매핑
    HashMap<AssetId, TypeId> id_to_type;

    // 소스 파일별 에셋 목록 (파일 단위 조회용)
    HashMap<Path, Array<AssetEntry>> file_to_assets;

    // AssetId -> Metadata (Editor가 .meta 파싱 후 Insert)
    HashMap<AssetId, AssetMetadata> id_to_meta;
};
}  // namespace se::asset
