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

    // Registry 스냅샷 복원 시도 (성공 시 Hot Start, 실패 시 Cold Start)
    const bool is_hot_start = LoadRegistrySnapshot();

    // VFS "Assets" 스킴에 마운트된 디렉토리를 스캔
    // TODO: [VPath] 스캔 대상 스킴을 설정 파일에서 읽도록 변경
    VFS::Get().VisitMounts([&](StringView scheme, const Path& physical_path, int32)
    {
        if (!scheme.Contains("Assets"))
        {
            return;
        }

        ScanWorkspace(physical_path, is_hot_start);
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

void EditorAssetSubsystem::ScanWorkspace(const Path& root_path, bool is_hot_start)
{
    ZoneScopedN("EditorAssetSubsystem::ScanWorkspace");

    if (!root_path.Exists() || !root_path.IsDirectory())
    {
        ConsoleLog(ELogLevel::Warning, "ScanWorkspace: Invalid directory: {}", root_path);
        return;
    }

    asset::AssetRegistry& registry = asset_subsystem->GetRegistry();

    // 삭제된 파일 감지를 위한 Set
    HashSet<Path> found_files;

    Array<Path> stack;
    stack.Push(root_path);

    uint32 new_count = 0;
    uint32 dirty_count = 0;
    uint32 clean_count = 0;

    // 디렉토리 순회 및 파일 상태 검사
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

            // 발견된 파일 기록 (Hot Start 시 고아 파일 감지용)
            if (is_hot_start)
            {
                found_files.Insert(entry_path);
            }

            // .meta 파일 보장
            Optional content_opt = EnsureMetaFile(entry_path);
            if (!content_opt.HasValue())
            {
                continue;
            }

            const asset::AssetMetadata& meta = content_opt->metadata;

            // 상태 판별
            const bool is_new = meta.sub_assets.IsEmpty();
            const bool is_dirty = !is_new && IsAssetDirty(entry_path, meta);

            if (is_new || is_dirty)
            {
                // TODO: [Phase 6] BackgroundWorker::PushCookTask(entry_path) 호출하여 백그라운드 굽기
                if (is_new)
                {
                    ++new_count;
                }
                if (is_dirty)
                {
                    ++dirty_count;
                }
            }
            else
            {
                ++clean_count;
            }

            // Registry에 등록 (Sub-asset이 비어있으면 함수 내부에서 알아서 스킵됨)
            RegisterFromMeta(entry_path, meta);
        }
    }

    // 삭제된 파일 감지 (Hot Start 전용 로직)
    uint32 orphaned_count = 0;
    if (is_hot_start)
    {
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
            ++orphaned_count;
        }
    }

    ConsoleLog(
        ELogLevel::Info,
        "ScanWorkspace Complete [HotStart: {}]: new={}, dirty={}, clean={}, orphaned={} in: {}",
        is_hot_start, new_count, dirty_count, clean_count, orphaned_count, root_path
    );
}

Optional<MetaFileContent> EditorAssetSubsystem::EnsureMetaFile(const Path& source_path)
{
    ZoneScopedN("EditorAssetSubsystem::EnsureMetaFile");

    if (Optional existing_content = MetaFileManager::Load(source_path))
    {
        return existing_content;
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
        return NullOpt;
    }

    ConsoleLog(ELogLevel::Info, "Created .meta for: {}", source_path);
    return content;
}

void EditorAssetSubsystem::RegisterFromMeta(const Path& source_path, const asset::AssetMetadata& meta)
{
    ZoneScopedN("EditorAssetSubsystem::RegisterFromMeta");

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
    return "registry.bin";
}
} // namespace se::editor
