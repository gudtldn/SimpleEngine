// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppMemberFunctionMayBeConst
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
        if (!scheme.Contains("Assets"))
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
        ConsoleLog(ELogLevel::Warning, "ScanDirectory: Invalid directory: {}", root_path);
        return;
    }

    uint32 scanned_count = 0;

    Array<Path> stack;
    stack.Push(root_path);
    while (const Optional dir = stack.Pop())
    {
        for (const DirectoryEntry& entry : FileSystem::ReadDir(*dir))
        {
            const Path entry_path = entry.GetPath();

            // 디렉토리는 재귀적으로 스캔
            if (entry.IsDirectory())
            {
                stack.Push(entry_path);
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
    }

    ConsoleLog(ELogLevel::Info, "ScanDirectory: Registered {} assets from: {}", scanned_count, root_path);
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
        ConsoleLog(ELogLevel::Error, "Failed to create .meta for: {}", source_path);
        return false;
    }

    ConsoleLog(ELogLevel::Info, "Created .meta for: {}", source_path);
    return true;
}

void EditorAssetSubsystem::RegisterFromMeta(const Path& source_path)
{
    ZoneScopedN("EditorAssetSubsystem::RegisterFromMeta");

    const Optional content_opt = MetaFileManager::Load(source_path);
    if (!content_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "RegisterFromMeta: No .meta file for: {}", source_path);
        return;
    }

    const asset::AssetMetadata& meta = content_opt->metadata;
    asset::AssetRegistry& registry = asset_subsystem->GetRegistry();

    // Sub-asset 목록이 비어있으면 아직 Import가 안 된 상태이므로 스킵
    if (meta.sub_assets.IsEmpty())
    {
        return;
    }

    // 각 Sub-asset을 Registry에 등록
    for (const asset::SubAssetMeta& sub : meta.sub_assets)
    {
        const asset::AssetId asset_id{ sub.guid };
        // TODO: [VPath] 물리 경로 대신 VPath를 AssetPath에 사용하도록 마이그레이션
        asset::AssetPath asset_path{ source_path, sub.name };

        asset::AssetMetadata sub_meta = meta;
        sub_meta.guid = sub.guid;

        registry.RegisterAsset(asset_id, sub.type, std::move(asset_path), std::move(sub_meta));
    }
}

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
    updated_content.metadata.source_hash = source_hash;
    updated_content.metadata.source_mtime = file_mtime;
    updated_content.metadata.source_size = file_size;
    updated_content.metadata.cache_version = current_cache_version;
    updated_content.metadata.sub_assets.Clear();

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
            .type = asset_type,
        });

        // Registry 등록
        asset::AssetMetadata sub_meta = updated_content.metadata;
        sub_meta.guid = asset_id.GetGuid();

        registry.RegisterAsset(asset_id, asset_type, std::move(asset_path), std::move(sub_meta));

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
        // TODO: [Phase 5] AssetDependency 추적 — import 결과의 의존 파일 목록을 DependencyGraph에 등록
        // TODO: [Phase 7] Hot-reload 시 AssetCache::FindOrCreate + ExchangeAsset으로 메모리 교체
    }

    // .meta 파일 갱신 (Sub-asset 정보 기록)
    if (!MetaFileManager::Save(file_path, updated_content))
    {
        ConsoleLog(ELogLevel::Warning, "CookAsset: Failed to update .meta for: {}", file_path);
    }

    ConsoleLog(ELogLevel::Info, "Successfully cooked {} assets from: {}", result.GetCount(), file_path);
    return true;
}

void EditorAssetSubsystem::ScanAndReconcile(const Path& root_path)
{
    ZoneScopedN("EditorAssetSubsystem::ScanAndReconcile");

    auto& registry = asset_subsystem->GetRegistry();

    // 파일 시스템에서 발견된 Import 가능 파일을 추적
    HashSet<Path> found_files;

    // 반복적 DFS로 디렉토리 순회 (깊은 계층 구조에서 스택 오버플로 방지)
    Array<Path> stack;
    stack.Push(root_path);

    uint32 new_count = 0;
    uint32 dirty_count = 0;

    while (const Optional dir = stack.Pop())
    {
        for (const DirectoryEntry& entry : FileSystem::ReadDir(*dir))
        {
            if (entry.IsDirectory())
            {
                stack.Push(entry.GetPath());
                continue;
            }

            if (!entry.IsFile())
            {
                continue;
            }

            const Path file_path = entry.GetPath();

            // .meta 파일 자체는 스킵
            if (file_path.Extension() == ".meta")
            {
                continue;
            }

            // Import 불가능한 파일은 스킵
            if (!importer->CanImport(file_path))
            {
                continue;
            }

            found_files.Insert(file_path);

            if (!MetaFileManager::HasMeta(file_path))
            {
                // 새 파일: .meta 생성 -> Registry 등록
                // TODO: [Phase 6] CookAsset을 Background thread로 dispatch
                EnsureMetaFile(file_path);
                RegisterFromMeta(file_path);
                ++new_count;
                continue;
            }

            // 기존 파일: 변경 여부 확인
            const Optional content_opt = MetaFileManager::Load(file_path);
            if (!content_opt.HasValue())
            {
                continue;
            }

            if (IsAssetDirty(file_path, content_opt->metadata))
            {
                // TODO: [Phase 6] Dirty 에셋을 Background thread cook queue에 추가
                ConsoleLog(ELogLevel::Info, "Dirty asset detected: {}", file_path);
                ++dirty_count;
            }

            // Registry에 등록 (아직 없다면 — Sub-asset이 비어있으면 RegisterFromMeta가 스킵)
            RegisterFromMeta(file_path);
        }
    }

    // 삭제된 파일 감지: Registry에 있지만 파일 시스템에 없는 에셋 제거
    Array<Path> orphaned;
    registry.VisitAllPaths([&found_files, &orphaned](const Path& registered_path)
    {
        if (!found_files.Contains(registered_path))
        {
            orphaned.Push(registered_path);
        }
    });

    for (const Path& path : orphaned)
    {
        ConsoleLog(ELogLevel::Warning, "Asset file deleted (offline): {}", path);
        registry.UnregisterByPath(path);
        MetaFileManager::DeleteMeta(path);
    }

    ConsoleLog(
        ELogLevel::Info,
        "ScanAndReconcile: new={}, dirty={}, orphaned={} in: {}",
        new_count, dirty_count, orphaned.Len(), root_path
    );
}

bool EditorAssetSubsystem::IsAssetDirty(const Path& source_path, const asset::AssetMetadata& meta) const
{
    // Quick reject: mtime이 동일하면 변경 없음
    const uint64 current_mtime = FileSystem::LastWriteTime(source_path).ValueOrDefault();
    if (current_mtime == meta.source_mtime)
    {
        return false;
    }

    // mtime이 다르면 size 비교 (size가 달라지면 확실히 변경됨)
    const uint64 current_size = static_cast<uint64>(FileSystem::FileSize(source_path).ValueOrDefault());
    if (current_size != meta.source_size)
    {
        return true;
    }

    // mtime 변경 + size 동일 -> SHA256 해시로 최종 확인
    // (git branch 전환, touch 등으로 mtime만 바뀐 경우 불필요한 reimport 방지)
    const String current_hash = SHA256::HashFile(source_path);
    return current_hash != meta.source_hash;
}

void EditorAssetSubsystem::SaveRegistrySnapshot()
{
    ZoneScopedN("EditorAssetSubsystem::SaveRegistrySnapshot");

    const asset::AssetRegistry& registry = asset_subsystem->GetRegistry();
    const Path snapshot_path = GetRegistrySnapshotPath();

    if (registry.SaveToFile(snapshot_path))
    {
        ConsoleLog(ELogLevel::Info, "Registry snapshot saved: {}", snapshot_path);
    }
}

bool EditorAssetSubsystem::LoadRegistrySnapshot()
{
    ZoneScopedN("EditorAssetSubsystem::LoadRegistrySnapshot");

    const Path snapshot_path = GetRegistrySnapshotPath();
    if (!snapshot_path.Exists())
    {
        return false;
    }

    return asset_subsystem->GetRegistry().LoadFromFile(snapshot_path);
}

Path EditorAssetSubsystem::GetRegistrySnapshotPath()
{
    // TODO: [VPath] VFS를 통해 프로젝트 빌드 디렉토리를 resolve하도록 변경
    return Path{ "Build" } / "registry.bin";
}
} // namespace se::editor
