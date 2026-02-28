#include "Asset/EditorAssetSubsystem.h"

#include "SimpleEditor/Asset/MetaFileContent.h"
#include "SimpleEditor/Asset/MetaFileManager.h"
#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"
#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Translators/AssimpTranslator.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SHA256.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorAssetSubsystem)
    .DependsOn<se::asset::AssetSubsystem>();

SE_BEGIN_REFLECT(EditorAssetSubsystem, meta::Internal)
SE_END_REFLECT(EditorAssetSubsystem)


EditorAssetSubsystem::EditorAssetSubsystem() = default;
EditorAssetSubsystem::~EditorAssetSubsystem() = default;

bool EditorAssetSubsystem::Initialize()
{
    {
        // Create AssetImporter Instance
        importer = std::make_unique<AssetImporter>();

        // Register Translators
        importer->RegisterTranslator<AssimpTranslator>();

        // Register Factories
        importer->RegisterFactory<StaticMeshFactory>();
    }

    asset_subsystem = &GetSubsystemChecked<asset::AssetSubsystem>();
    asset_subsystem->SetDDCMissHandler([this](asset::AssetSubsystem&, const Path& file_path) -> bool
    {
        return CookAsset(file_path);
    });

    // Registry 스냅샷 복원 시도
    const bool snapshot_loaded = LoadRegistrySnapshot();

    // VFS "Assets" 스킴에 마운트된 디렉토리를 스캔
    // TODO: [VPath] 스캔 대상 스킴을 설정 파일에서 읽도록 변경
    VFS::Get().VisitMounts([&](StringView scheme, const Path& physical_path, int32)
    {
        if (scheme != "Assets")
        {
            return;
        }

        if (snapshot_loaded)
        {
            // Hot Start: 스냅샷과 파일 시스템 비교
            ScanAndReconcile(physical_path);
        }
        else
        {
            // Cold Start: 전체 스캔
            ScanDirectory(physical_path);
        }
    });

    return true;
}

void EditorAssetSubsystem::Release()
{
    // 에디터 종료 시 Registry 스냅샷 저장
    SaveRegistrySnapshot();

    if (asset_subsystem)
    {
        asset_subsystem->SetDDCMissHandler(nullptr);
    }
    importer.reset();
}

void EditorAssetSubsystem::ScanDirectory(const Path& root_path)
{
    ZoneScopedN("EditorAssetSubsystem::ScanDirectory");

    if (!root_path.Exists() || !root_path.IsDirectory())
    {
        ConsoleLog(ELogLevel::Warning, "ScanDirectory: Invalid directory: {}", root_path.ToString());
        return;
    }

    uint32 scanned_count = 0;
    for (const DirectoryEntry& entry : FileSystem::ReadDir(root_path))
    {
        const Path entry_path = entry.GetPath();

        // 디렉토리는 재귀적으로 스캔
        if (entry.IsDirectory())
        {
            ScanDirectory(entry_path);
            continue;
        }

        // 일반 파일이 아닌 경우 스킵
        if (!entry.IsFile())
        {
            continue;
        }

        // .meta 파일 자체는 스킵
        const Optional ext = entry_path.Extension();
        if (ext == ".meta")
        {
            continue;
        }

        // Import 불가능한 파일은 스킵
        if (!importer->CanImport(entry_path))
        {
            continue;
        }

        // .meta 파일 보장
        if (!EnsureMetaFile(entry_path))
        {
            continue;
        }

        // Registry에 등록
        RegisterFromMeta(entry_path);
        ++scanned_count;
    }

    ConsoleLog(ELogLevel::Info, "ScanDirectory: Registered {} assets from: {}", scanned_count, root_path.ToString());
}

bool EditorAssetSubsystem::EnsureMetaFile(const Path& source_path)
{
    ZoneScopedN("EditorAssetSubsystem::EnsureMetaFile");

    if (MetaFileManager::HasMeta(source_path))
    {
        return true;
    }

    // 새 MetaFileContent 생성
    MetaFileContent content;
    content.metadata = {
        .guid = Guid::NewGuid(),
        .source_hash = SHA256::HashFile(source_path),
        .source_mtime = FileSystem::LastWriteTime(source_path).ValueOrDefault(),
        .source_size = static_cast<uint64>(FileSystem::FileSize(source_path).ValueOrDefault()),
        .cache_version = 1,
    };

    // ImportProfile은 기본값 (빈 프로파일)으로 생성
    // TODO: ImportPresetManager에서 Translator별 기본 프리셋 로드

    if (!MetaFileManager::Save(source_path, content))
    {
        ConsoleLog(ELogLevel::Error, "Failed to create .meta for: {}", source_path.ToString());
        return false;
    }

    ConsoleLog(ELogLevel::Info, "Created .meta for: {}", source_path.ToString());
    return true;
}

void EditorAssetSubsystem::RegisterFromMeta(const Path& source_path)
{
    ZoneScopedN("EditorAssetSubsystem::RegisterFromMeta");

    const Optional content_opt = MetaFileManager::Load(source_path);
    if (!content_opt.HasValue())
    {
        return;
    }

    const asset::AssetMetadata& meta = content_opt->metadata;
    auto& registry = asset_subsystem->GetRegistry();

    // Sub-asset 목록이 비어있으면 아직 Import가 안 된 상태이므로 스킵
    if (meta.sub_assets.IsEmpty())
    {
        return;
    }

    // 각 Sub-asset을 Registry에 등록
    for (const auto& sub : meta.sub_assets)
    {
        const asset::AssetId asset_id{ sub.guid };
        const asset::AssetPath asset_path{ source_path, sub.name };

        registry.RegisterAsset(asset_id, sub.type, asset_path, meta);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool EditorAssetSubsystem::CookAsset(const Path& file_path)
{
    ZoneScopedN("EditorAssetSubsystem::CookAsset");

    // .meta에서 ImportProfile 획득 (없으면 기본값)
    Optional meta_content_opt = MetaFileManager::Load(file_path);
    ImportProfile import_profile = meta_content_opt
        .Map([](const MetaFileContent& content)
        {
            return content.import_settings;
        })
        .ValueOrDefault();

    // Import 수행
    const auto result_exp = importer->Import(file_path, import_profile);
    if (!result_exp.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Cook failed: {}", result_exp.Error().What());
        return false;
    }
    const ImportResult& result = result_exp.Value();

    // 메타데이터 및 해시 계산
    const String source_hash = SHA256::HashFile(file_path); // TODO: 나중에 xxHash로 변경
    constexpr uint32 current_cache_version = 1;
    const uint64 file_mtime = FileSystem::LastWriteTime(file_path).ValueOrDefault();
    const uint64 file_size = static_cast<uint64>(FileSystem::FileSize(file_path).ValueOrDefault());

    auto& registry = asset_subsystem->GetRegistry();
    auto& ddc = asset_subsystem->GetDDC();

    // MetaFileContent 갱신 준비
    MetaFileContent updated_content = std::move(meta_content_opt).ValueOrDefault();
    updated_content.metadata = {
        .source_hash = source_hash,
        .source_mtime = file_mtime,
        .source_size = file_size,
        .cache_version = current_cache_version,
    };

    // Primary GUID 보장
    if (!updated_content.metadata.guid.IsValid())
    {
        updated_content.metadata.guid = Guid::NewGuid();
    }

    // Registry 및 DDC 갱신
    for (const auto& [name, idx] : result.GetNameToIndexMap())
    {
        std::shared_ptr<asset::AssetBase> asset = result.GetAsset(idx);
        if (!asset)
        {
            continue;
        }

        const TypeId asset_type = asset->GetTypeId();
        asset::AssetPath asset_path = asset::AssetPath{ file_path, name };

        // 기존 ID 재사용 또는 새 GUID 발급
        asset::AssetId asset_id = registry.GetAssetId(asset_path).ValueOr(asset::AssetId{ Guid::NewGuid() });

        // Meta에 Sub-asset 정보 추가
        updated_content.metadata.sub_assets.Push({
            .name = name,
            .guid = asset_id.GetGuid(),
            .type = asset_type
        });

        // Registry 등록
        registry.RegisterAsset(asset_id, asset_type, asset_path, updated_content.metadata);

        // DDC 굽기 (직렬화)
        if (!source_hash.IsEmpty())
        {
            Array<uint8> payload = asset::AssetSubsystem::SerializeAssetPayload(*asset);
            if (!payload.IsEmpty())
            {
                ddc.Store(asset_id.GetGuid(), {
                    .source_hash = source_hash,
                    .cache_version = current_cache_version,
                    .payload = std::move(payload),
                });
            }
        }

        // AssetCache 등록은 Callback 후 자동으로 이루어짐
        // [AssetSubsystem::LoadInternal 참고]
    }

    // .meta 파일 갱신 (Sub-asset 정보 기록)
    if (!MetaFileManager::Save(file_path, updated_content))
    {
        ConsoleLog(ELogLevel::Warning, "CookAsset: Failed to update .meta for: {}", file_path.ToString());
    }

    ConsoleLog(ELogLevel::Info, "Successfully cooked {} assets from: {}", result.GetCount(), file_path);
    return true;
}
} // namespace se::editor
