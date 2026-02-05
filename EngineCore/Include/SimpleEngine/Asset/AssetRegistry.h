#pragma once
#include <shared_mutex>

#include "SimpleEngine/Asset/AssetId.h"
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
    String sub_name;
};

/**
 * Asset의 Path와 Id 매핑을 관리하는 클래스
 * @todo 나중에 .meta 기반 레지스트리로 변경
 */
class SE_CORE_API AssetRegistry
{
public:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    // 복사 & 이동 금지
    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;
    AssetRegistry(AssetRegistry&&) = delete;
    AssetRegistry& operator=(AssetRegistry&&) = delete;

public:
    /** 새로운 Asset을 Registry에 등록합니다. */
    void RegisterAsset(const AssetId& asset_id, const TypeId& asset_type, AssetPath&& asset_path);

    /** AssetPath에 해당하는 AssetId를 반환합니다. */
    [[nodiscard]] Optional<const AssetId&> GetAssetId(const AssetPath& asset_path) const;

    /** AssetId에 해당하는 AssetPath를 반환합니다. */
    [[nodiscard]] Optional<const AssetPath&> GetAssetPath(const AssetId& asset_id) const;

    /** 파일 내에서 특정 타입의 첫 번째 Asset ID를 찾습니다. */
    [[nodiscard]] Optional<const AssetId&> FindFirstOfType(const Path& file_path, const TypeId& type) const;

    /** 파일에 등록된 모든 Asset 목록을 반환합니다. */
    Optional<const Array<AssetEntry>&> GetAssetsInFile(const Path& file_path) const;

    /** Path에 있는 파일을 Import 한걸로 표시합니다. */
    void MarkFileAsImported(const Path& file_path);

    /** Path에 있는 파일 */
    [[nodiscard]] bool IsFileImported(const Path& file_path) const;

private:
    mutable TracySharedLockable(std::shared_mutex, registry_mutex);

    // Import된 파일 추적 (중복 Import 방지)
    HashSet<Path> imported_files;

    // Path <-> AssetId 매핑
    HashMap<AssetPath, AssetId> path_to_id;
    HashMap<AssetId, AssetPath> id_to_path;

    // Path별 에셋 목록
    HashMap<Path, Array<AssetEntry>> file_to_assets;
};
}  // namespace se::asset
