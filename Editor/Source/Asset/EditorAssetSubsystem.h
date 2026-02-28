#pragma once

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"

#include <memory>


namespace se::editor
{
// forward declarations
class AssetImporter;

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
     * 디렉토리를 재귀적으로 순회하며, Import 가능한 파일에 대해
     * .meta 파일 보장 및 AssetRegistry 등록을 수행합니다.
     * @param root_path 스캔할 루트 디렉토리 경로
     */
    void ScanDirectory(const Path& root_path);

    /**
     * 소스 파일에 대한 .meta 파일이 존재하는지 확인하고,
     * 없으면 새로 생성합니다.
     * @param source_path 소스 파일 경로
     * @return .meta 파일이 존재하거나 생성에 성공하면 true
     */
    bool EnsureMetaFile(const Path& source_path);

private:
    /**
     * .meta 파일에서 메타데이터를 읽어 AssetRegistry에 등록합니다.
     * @param source_path 소스 파일 경로
     */
    void RegisterFromMeta(const Path& source_path);

    /**
     * Asset을 Import하여 Registry 등록과 DDC 바이너리를 생성합니다.
     * .meta 파일이 존재하면 ImportProfile을 획득하여 Import에 전달합니다.
     * @param file_path 소스 파일 경로
     * @return 성공 여부
     */
    bool CookAsset(const Path& file_path);

private:
    std::unique_ptr<AssetImporter> importer;
    asset::AssetSubsystem* asset_subsystem = nullptr;
};
}  // namespace se::editor
