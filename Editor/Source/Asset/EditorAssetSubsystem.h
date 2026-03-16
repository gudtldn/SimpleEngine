#pragma once

#include "SimpleEditor/Asset/DependencyGraph.h"
#include "SimpleEditor/Asset/ImportPresetManager.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "tracy/Tracy.hpp"

#include <memory>
#include <mutex>


namespace se::editor
{
// forward declarations
class AssetImporter;
struct MetaFileContent;

/**
 * Editor 전용 에셋 서브시스템
 *
 * Core의 AssetSubsystem에 DDCMissHandler를 등록하여
 * Import 파이프라인을 연결하고, .meta 파일 관리 및 디렉토리 스캔 기능을 제공합니다.
 */
class SE_ANNOTATION(=meta::Internal) EditorAssetSubsystem : public SubsystemBase
{
    SE_CLASS(EditorAssetSubsystem, SubsystemBase)

public:
    EditorAssetSubsystem();
    virtual ~EditorAssetSubsystem() override;

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

public:
    /**
     * 작업 공간(Workspace)의 디렉토리를 순회하며 에셋의 상태를 스캔하고 레지스트리를 갱신합니다.
     * Import 가능한 파일에 대해 .meta 파일을 보장하고, 변경(New/Dirty) 상태를 감지하여 등록합니다.
     * @param root_path 스캔할 루트 디렉토리의 물리 경로
     * @param is_hot_start true일 경우 레지스트리 스냅샷과 비교하여 삭제된(Orphaned) 에셋을 찾아 등록 해제합니다.
     */
    void ScanWorkspace(const Path& root_path, bool is_hot_start);

    /**
     * 소스 파일에 대한 .meta 파일이 존재하는지 확인하고,
     * 없으면 새로 생성합니다.
     * @param source_path 소스 파일의 물리 경로
     * @return .meta 파일이 존재하거나 생성에 성공하면 MetaFileContent
     */
    [[nodiscard]] Optional<MetaFileContent> EnsureMetaFile(const Path& source_path);

    /**
     * Asset을 Import하여 Registry 등록과 DDC 바이너리를 생성합니다.
     * .meta 파일이 존재하면 ImportProfile을 획득하여 Import에 전달합니다.
     * @param file_vpath 소스 파일의 가상 경로
     * @return 성공 여부
     */
    bool CookAsset(const VPath& file_vpath);

    /** DependencyGraph에 대한 읽기 전용 접근자 */
    [[nodiscard]] const DependencyGraph& GetDependencyGraph() const { return dep_graph; }

private:
    /**
     * .meta 파일에서 메타데이터를 읽어 AssetRegistry에 등록합니다.
     * @param source_vpath 소스 파일의 가상 경로
     * @param meta 메타데이터
     */
    void RegisterFromMeta(const VPath& source_vpath, const asset::AssetMetadata& meta);

    /**
     * 소스 파일의 mtime/size와 .meta의 기록값을 비교하여 변경 여부를 판별합니다.
     * mtime 비교 -> size 비교 -> hash 비교 순서의 단계적 검증을 수행합니다.
     * @param source_path 소스 파일의 물리 경로
     * @param meta 저장된 메타데이터
     * @return 소스 파일이 수정되었으면 true
     */
    [[nodiscard]] bool IsAssetDirty(const Path& source_path, const asset::AssetMetadata& meta) const;

    /** Registry를 바이너리 파일로 저장합니다. (에디터 종료 시 호출) */
    void SaveRegistrySnapshot();

    /** 바이너리 스냅샷에서 Registry를 복원합니다. */
    [[nodiscard]] bool LoadRegistrySnapshot();

    /** Registry 스냅샷 파일의 가상 경로를 반환합니다. */
    [[nodiscard]] static VPath GetRegistrySnapshotVPath();

    /**
     * Registry에 등록된 모든 에셋의 메타데이터를 읽어 DependencyGraph를 구축합니다.
     * ScanWorkspace 완료 직후 호출됩니다.
     */
    void BuildDependencyGraph();

    /**
     * 단일 에셋의 의존성 항목을 AssetId로 변환하여 DependencyGraph에 동기화합니다.
     * @param asset_id 의존성을 갱신할 에셋의 ID
     * @param dependencies .meta에서 읽은 의존성 항목 목록
     */
    void SyncDependencies(
        const asset::AssetId& asset_id,
        const Array<asset::AssetDependencyEntry>& dependencies
    );

private:
    std::unique_ptr<AssetImporter> importer;
    asset::AssetSubsystem* asset_subsystem = nullptr;
    ImportPresetManager preset_manager;
    DependencyGraph dep_graph;

    TracyLockable(std::mutex, cooking_mutex);
    HashSet<VPath> currently_cooking;
};
}  // namespace se::editor
