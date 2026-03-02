// ReSharper disable CppMemberFunctionMayBeStatic
// ReSharper disable CppMemberFunctionMayBeConst
#include "Asset/EditorAssetSubsystem.h"

#include "SimpleEditor/Asset/MetaFileContent.h"
#include "SimpleEditor/Asset/MetaFileManager.h"
#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"
#include "SimpleEditor/Asset/Pipeline/PipelineProcessorStack.h"
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
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
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

    // Translator별 기본 ImportProfile 프리셋 등록
    preset_manager.RegisterPreset(TypeId::Get<AssimpTranslator>(), [](ImportProfile& profile)
    {
        profile.Emplace<MeshImportSettings>();
    });

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

    // === 디렉토리 순회, 파일 분류 ===
    struct OrphanMeta
    {
        Path source_path; // 빈 경로면 소비됨(이동 매칭 완료) 표시
        MetaFileContent content;
    };

    Array<Path> source_files;
    Array<OrphanMeta> orphan_metas;
    HashSet<Path> found_files;

    {
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

                // 실제 Source 파일도 같이 존재하는지 확인
                const Optional ext = entry_path.Extension();
                if (ext == ".meta")
                {
                    // .meta 파일의 소스 파일 존재 여부 확인 -> 없으면 고아 .meta
                    Path source = MetaFileManager::GetSourcePath(entry_path);
                    if (!source.Exists())
                    {
                        if (Optional content = MetaFileManager::Load(source))
                        {
                            orphan_metas.Push({
                                .source_path = std::move(source),
                                .content = std::move(content).Value(),
                            });
                        }
                    }
                    continue;
                }

                // Import 불가능한 파일은 스킵
                if (!importer->CanImport(entry_path))
                {
                    continue;
                }

                source_files.Push(entry_path);
                if (is_hot_start)
                {
                    found_files.Insert(entry_path);
                }
            }
        }
    }

    // === 고아 .meta 해시 인덱스 구축 (이동 감지용) ===
    HashMap<String, uint32> orphan_by_hash;
    for (const auto [n, orphan_meta] : orphan_metas | std::views::enumerate)
    {
        const String& hash = orphan_meta.content.metadata.source_hash;
        if (!hash.IsEmpty())
        {
            orphan_by_hash.Insert(hash, static_cast<uint32>(n));
        }
    }

    // === 소스 파일별 처리 ===
    uint32 new_count = 0;
    uint32 dirty_count = 0;
    uint32 clean_count = 0;
    uint32 moved_count = 0;

    for (const Path& file_path : source_files)
    {
        Optional<MetaFileContent> content_opt;

        if (MetaFileManager::HasMeta(file_path))
        {
            // 기존 .meta 로드
            content_opt = MetaFileManager::Load(file_path);
        }
        else if (!orphan_by_hash.IsEmpty())
        {
            // .meta 없음 -> 해시 매칭으로 오프라인 이동 감지
            const String hash = SHA256::HashFile(file_path);
            if (const Optional idx = orphan_by_hash.Find(hash))
            {
                OrphanMeta& orphan = orphan_metas[*idx];

                ConsoleLog(ELogLevel::Info, "Asset moved (offline): {} -> {}", orphan.source_path, file_path);

                // 고아 .meta의 GUID를 계승하여 새 위치에 저장
                MetaFileContent adopted = std::move(orphan.content);
                adopted.metadata.source_mtime = FileSystem::LastWriteTime(file_path).ValueOrDefault();
                adopted.metadata.source_size = static_cast<uint64>(FileSystem::FileSize(file_path).ValueOrDefault());

                MetaFileManager::Save(file_path, adopted);
                MetaFileManager::DeleteMeta(orphan.source_path);

                orphan.source_path = {}; // 소비됨 표시
                orphan_by_hash.Remove(hash);
                ++moved_count;

                content_opt = std::move(adopted);
            }
        }

        // 이동 매칭 실패 시 새 .meta 생성
        if (!content_opt.HasValue())
        {
            content_opt = EnsureMetaFile(file_path);
        }

        if (!content_opt.HasValue())
        {
            continue;
        }

        const asset::AssetMetadata& meta = content_opt->metadata;
        const bool is_new = meta.sub_assets.IsEmpty();
        const bool is_dirty = !is_new && IsAssetDirty(file_path, meta);

        if (is_new || is_dirty)
        {
            // TODO: BackgroundWorker::PushCookTask(file_path) 호출하여 백그라운드 굽기
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

        RegisterFromMeta(file_path, meta);
    }

    // === 삭제된 파일 감지 (Hot Start 전용) ===
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

    // === 매칭되지 않은 고아 .meta 정리 ===
    uint32 orphan_meta_count = 0;
    for (const OrphanMeta& orphan : orphan_metas)
    {
        if (!orphan.source_path.IsEmpty())
        {
            ConsoleLog(ELogLevel::Info, "Deleted orphan .meta: {}", MetaFileManager::GetMetaPath(orphan.source_path));
            MetaFileManager::DeleteMeta(orphan.source_path);
            ++orphan_meta_count;
        }
    }

    ConsoleLog(
        ELogLevel::Info,
        "ScanWorkspace Complete [HotStart: {}]: new={}, dirty={}, clean={}, moved={}, orphaned={}, orphan_meta={} in: {}",
        is_hot_start, new_count, dirty_count, clean_count, moved_count, orphaned_count, orphan_meta_count, root_path
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

    // Translator에 맞는 기본 ImportProfile 설정
    if (const Optional translator_type = importer->FindTranslatorTypeId(source_path))
    {
        content.import_settings = preset_manager.GetDefaultProfile(*translator_type);
    }

    if (!MetaFileManager::Save(source_path, content))
    {
        ConsoleLog(ELogLevel::Error, "Failed to create .meta for: {}", source_path);
        return NullOpt;
    }

    ConsoleLog(ELogLevel::Info, "Created .meta for: {}", source_path);
    return content;
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

    // .meta에서 ProcessorStack 복원
    Optional<PipelineProcessorStack> stack_opt;
    if (meta_content_opt.HasValue() && !meta_content_opt->processor_stack.IsEmpty())
    {
        PipelineProcessorStack stack;
        const TypeRegistry& type_registry = TypeRegistry::Get();

        for (const ProcessorEntry& entry : meta_content_opt->processor_stack)
        {
            if (!entry.enabled)
            {
                continue;
            }

            const Optional info_opt = type_registry.Find(entry.processor_type);
            if (!info_opt.HasValue() || !info_opt->constructor)
            {
                ConsoleLog(
                    ELogLevel::Warning,
                    "CookAsset: Processor type not found or not constructible: {}",
                    entry.processor_type.GetName()
                );
                continue;
            }

            IPipelineProcessor* raw = static_cast<IPipelineProcessor*>(info_opt->constructor());
            stack.AddProcessor(std::unique_ptr<IPipelineProcessor>(raw));
        }

        stack_opt.Emplace(std::move(stack));
    }

    // Import 수행
    const auto result_exp = importer->Import(file_path, import_profile, stack_opt);
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

    asset::AssetRegistry& registry = asset_subsystem->GetRegistry();
    asset::DerivedDataCache& ddc = asset_subsystem->GetDDC();

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
        // TODO: AssetDependency 추적 — import 결과의 의존 파일 목록을 DependencyGraph에 등록
        // TODO: Hot-reload 시 AssetCache::FindOrCreate + ExchangeAsset으로 메모리 교체
    }

    // .meta 파일 갱신 (Sub-asset 정보 기록)
    if (!MetaFileManager::Save(file_path, updated_content))
    {
        ConsoleLog(ELogLevel::Warning, "CookAsset: Failed to update .meta for: {}", file_path);
    }

    ConsoleLog(ELogLevel::Info, "Successfully cooked {} assets from: {}", result.GetCount(), file_path);
    return true;
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
