#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "tracy/Tracy.hpp"

#include <concepts>
#include <shared_mutex>


namespace se::asset
{
/**
 * AssetRegistry 내부에 저장되는 Asset의 전체 정보를 나타내는 구조체
 */
struct SE_ANNOTATION(=meta::SerializeOnly) AssetRecord
{
    SE_ANNOTATION(=meta::Property)
    AssetId id;

    SE_ANNOTATION(=meta::Property)
    TypeId type;

    SE_ANNOTATION(=meta::Property)
    AssetPath logical_path;

    SE_ANNOTATION(=meta::Property)
    AssetMetadata metadata;
};

/**
 * Asset의 Path/Id/Type/Meta 매핑을 관리하는 중앙 레지스트리
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
    /**
     * Asset의 모든 정보를 Registry에 등록합니다.
     * @param asset_id 등록할 Asset의 식별자
     * @param asset_type 등록할 Asset의 타입
     * @param asset_path 등록할 Asset의 경로
     * @param meta 등록할 Asset의 Metadata
     */
    void RegisterAsset(
        const AssetId& asset_id, const TypeId& asset_type,
        AssetPath asset_path, AssetMetadata meta
    );

    /**
     * 등록된 Asset을 Registry에서 제거합니다.
     * @param asset_id 제거할 Asset의 식별자
     */
    void UnregisterAsset(const AssetId& asset_id);

    /** Registry의 모든 데이터를 초기화합니다. */
    void Clear();

public:
    /**
     * AssetId로 Record를 찾아 Callback을 호출합니다.
     * @param asset_id 조회할 Asset ID
     * @param callback 레코드를 처리할 람다 함수 `void(const AssetRecord&)`
     * @return 에셋이 존재하여 콜백이 실행되었으면 true
     */
    template <typename Fn>
        requires std::invocable<Fn, const AssetRecord&>
    bool ReadRecord(const AssetId& asset_id, Fn&& callback) const;

    /** AssetPath에 해당하는 AssetId를 반환합니다. */
    [[nodiscard]] Optional<AssetId> GetAssetId(const AssetPath& asset_path) const;

    /** AssetId에 해당하는 TypeId를 반환합니다. */
    [[nodiscard]] Optional<TypeId> GetAssetType(const AssetId& asset_id) const;

    /** 파일 내에서 특정 타입의 첫 번째 Asset ID를 찾습니다. */
    [[nodiscard]] Optional<AssetId> FindFirstOfType(const VPath& file_path, const TypeId& type) const;

    /** 파일에 등록된 모든 sub-asset의 AssetId 목록을 반환합니다. */
    [[nodiscard]] Array<AssetId> GetAssetsInFile(const VPath& file_path) const;

    /** VPath에 있는 파일이 Import되었는지 확인합니다. */
    [[nodiscard]] bool IsFileImported(const VPath& file_path) const;

    /** 현재 등록된 Asset의 총 개수를 반환합니다. */
    [[nodiscard]] uint32 GetAssetCount() const;

    /**
     * Registry에 등록된 모든 소스 파일 경로를 순회합니다.
     * @param visitor 각 소스 파일 경로에 대해 호출되는 콜백
     */
    void VisitAllPaths(const Function<void(const VPath&)>& visitor) const;

    /**
     * 소스 파일 경로에 연결된 모든 Sub-asset을 일괄 제거합니다.
     * @param source_path 제거할 소스 파일 경로 (VPath)
     */
    void UnregisterByPath(const VPath& source_path);

public:
    /**
     * Registry 전체를 바이너리 파일로 저장합니다.
     * @param file_path 저장할 바이너리 파일 경로
     * @return 저장 성공 여부
     */
    [[nodiscard]] bool SaveToFile(const Path& file_path) const;

    /**
     * 바이너리 파일에서 Registry를 복원합니다.
     * @param file_path 읽어올 바이너리 파일 경로
     * @return 로드 성공 여부
     */
    [[nodiscard]] bool LoadFromFile(const Path& file_path);

private:
    mutable TracySharedLockable(std::shared_mutex, registry_mutex);

    // AssetId로 Asset의 실제 데이터를 관리하는
    HashMap<AssetId, AssetRecord> records;

    // AssetPath를 통해 Asset의 AssetId를 조회하기 위한 역방향 인덱스
    HashMap<AssetPath, AssetId> path_to_id;

    // 소스 파일(VPath)별 sub-asset의 AssetId 목록
    HashMap<VPath, Array<AssetId>> file_to_assets;
};

template <typename Fn>
    requires std::invocable<Fn, const AssetRecord&>
bool AssetRegistry::ReadRecord(const AssetId& asset_id, Fn&& callback) const
{
    std::shared_lock lock(registry_mutex);
    if (const Optional record_opt = records.Find(asset_id))
    {
        std::forward<Fn>(callback)(*record_opt);
        return true;
    }
    return false;
}
} // namespace se::asset

SE_DECLARE_REFLECTION(se::asset::AssetRecord)
