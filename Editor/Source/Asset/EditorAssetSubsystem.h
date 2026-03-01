#pragma once

#include "SimpleEditor/Asset/ImportPresetManager.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"

#include <memory>


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
     * @param root_path 스캔할 루트 디렉토리 경로
     * @param is_hot_start true일 경우 레지스트리 스냅샷과 비교하여 삭제된(Orphaned) 에셋을 찾아 등록 해제합니다.
     */
    void ScanWorkspace(const Path& root_path, bool is_hot_start);

    /**
     * 소스 파일에 대한 .meta 파일이 존재하는지 확인하고,
     * 없으면 새로 생성합니다.
     * @param source_path 소스 파일 경로
     * @return .meta 파일이 존재하거나 생성에 성공하면 true
     */
    [[nodiscard]] Optional<MetaFileContent> EnsureMetaFile(const Path& source_path);

    /**
     * Asset을 Import하여 Registry 등록과 DDC 바이너리를 생성합니다.
     * .meta 파일이 존재하면 ImportProfile을 획득하여 Import에 전달합니다.
     * @param file_path 소스 파일 경로
     * @return 성공 여부
     */
    bool CookAsset(const Path& file_path);

private:
    /**
     * .meta 파일에서 메타데이터를 읽어 AssetRegistry에 등록합니다.
     * @param source_path 소스 파일 경로
     * @param meta
     */
    void RegisterFromMeta(const Path& source_path, const asset::AssetMetadata& meta);

    /**
     * 소스 파일의 mtime/size와 .meta의 기록값을 비교하여 변경 여부를 판별합니다.
     * mtime 비교 -> size 비교 -> hash 비교 순서의 단계적 검증을 수행합니다.
     * @param source_path 소스 파일 경로
     * @param meta 저장된 메타데이터
     * @return 소스 파일이 수정되었으면 true
     */
    [[nodiscard]] bool IsAssetDirty(const Path& source_path, const asset::AssetMetadata& meta) const;

    /** Registry를 바이너리 파일로 저장합니다. (에디터 종료 시 호출) */
    void SaveRegistrySnapshot();

    /** 바이너리 스냅샷에서 Registry를 복원합니다. */
    [[nodiscard]] bool LoadRegistrySnapshot();

    /** Registry 스냅샷 파일 경로를 반환합니다. */
    [[nodiscard]] static Path GetRegistrySnapshotPath();

private:
    std::unique_ptr<AssetImporter> importer;
    asset::AssetSubsystem* asset_subsystem = nullptr;
    ImportPresetManager preset_manager;
};
}  // namespace se::editor
