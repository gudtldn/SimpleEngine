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
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h"
#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/HAL/EventSubsystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/ScopedTimer.h"
#include "SimpleEngine/Utility/SHA256.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
namespace
{
/** ScanWorkspace에서 Dirty 파일을 병렬로 Cook하기 위한 코루틴 함수 */
JobTask<void> MakeCookTask(EditorAssetSubsystem& self, VPath vpath, std::atomic<usize>& completed, usize total)
{
    const ScopedTimer timer;
    const bool success = self.CookAsset(vpath);
    const usize n = completed.fetch_add(1, std::memory_order_relaxed) + 1;

    if (success)
    {
        ConsoleLog(ELogLevel::Info, "Cooked [{}/{}] {} ({:.1f}ms)", n, total, vpath, timer.ElapsedMs());
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "Cook failed [{}/{}] {} ({:.1f}ms)", n, total, vpath, timer.ElapsedMs());
    }
    co_return;
}
} // namespace


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
    preset_manager.RegisterPreset<AssimpTranslator>([](ImportProfile& profile)
    {
        profile.Emplace<MeshImportSettings>();
    });

    asset_subsystem = &GetSubsystemChecked<asset::AssetSubsystem>();
    asset_subsystem->SetDDCMissHandler([this](asset::AssetSubsystem&, const VPath& file_vpath) -> bool
    {
        return CookAsset(file_vpath);
    });

    // Registry 스냅샷 복원 시도 (성공 시 Hot Start, 실패 시 Cold Start)
    const bool is_hot_start = LoadRegistrySnapshot();

    // EditorConfig에서 스캔 대상 스킴 목록 로드
    AssetScanSettings scan_settings;
    if (auto config_result = ConfigFile::Load("Config://EditorConfig.toml"))
    {
        scan_settings = config_result->GetSection<AssetScanSettings>("asset_scan");
    }

    // 설정된 스킴에 마운트된 디렉토리를 스캔
    const HashSet<StringView> scan_schemes = HashSet<StringView>::FromRange(
        scan_settings.schemes | std::views::transform([](const String& scheme) -> StringView
        {
            return scheme;
        })
    );
    VFS::Get().VisitMounts([&](StringView scheme, const Path& physical_path, int32)
    {
        if (!scan_schemes.Contains(scheme))
        {
            return;
        }

        ScanWorkspace(physical_path, is_hot_start);
    });

    // OS 파일 드롭 이벤트 구독
    EventSubsystem& event_subsystem = GetSubsystemChecked<EventSubsystem>();
    file_drop_handle = event_subsystem.on_file_dropped.AddLambda([this](const Path& file_path)
    {
        ImportExternalFile(file_path);
    });

    // PropertyPanel D&D용 AssetDropResolver 등록
    DrawerRegistry::Get().SetAssetDropResolver([](const char* dropped_path) -> asset::AssetId
    {
        // dropped_path는 AssetsBrowserPanel에서 전달한 물리 경로
        // VFS 역변환 -> Registry에서 첫 번째 에셋 ID 조회
        const Optional file_vpath = VFS::Unresolve(Path{ dropped_path });
        if (!file_vpath)
        {
            return {};
        }

        const asset::AssetRegistry& registry = GetSubsystemChecked<asset::AssetSubsystem>().GetRegistry();
        const Array<asset::AssetId> assets = registry.GetAssetsInFile(*file_vpath);
        if (assets.IsEmpty())
        {
            return {};
        }
        return assets.Front().Value();
    });

    return true;
}

void EditorAssetSubsystem::Release()
{
    // 에디터 종료 시 Registry 스냅샷 저장
    SaveRegistrySnapshot();

    // 이벤트 구독 해제
    if (file_drop_handle.IsValid())
    {
        if (EventSubsystem* event_subsystem = GetSubsystem<EventSubsystem>())
        {
            event_subsystem->on_file_dropped.Remove(file_drop_handle);
        }
    }

    // AssetDropResolver 해제
    DrawerRegistry::Get().SetAssetDropResolver(nullptr);

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

    // === 디렉토리 순회, 파일 분류 ===
    struct OrphanMeta
    {
        Path source_path; // 빈 경로면 소비됨(이동 매칭 완료) 표시
        MetaFileContent content;
    };

    Array<Path> source_files;
    Array<OrphanMeta> orphan_metas;
    HashSet<VPath> found_vpaths;

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
                    if (Optional vpath_opt = VFS::Unresolve(entry_path))
                    {
                        found_vpaths.Insert(std::move(vpath_opt).Value());
                    }
                    else
                    {
                        ConsoleLog(ELogLevel::Warning, "ScanWorkspace: File is outside VFS bounds: {}", entry_path);
                    }
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

    // Background Cook 목록
    Array<VPath> dirty_vpaths;

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

        // 새로운 파일인 경우, .meta 파일 생성
        if (!content_opt.HasValue())
        {
            content_opt = EnsureMetaFile(file_path); // TODO: 새 파일이 많을 때 여기서 병목이 생김 (Blocking)

            // EnsureMetaFile이 실패한 경우 (로그는 내부에서 남김)
            if (!content_opt.HasValue())
            {
                continue;
            }
        }

        // VPath 변환 (Registry 등록 + cook dispatch 모두에 필요)
        Optional file_vpath = VFS::Unresolve(file_path);
        if (!file_vpath)
        {
            ConsoleLog(ELogLevel::Error, "ScanWorkspace: Fatal error, lost VFS tracking for: {}", file_path);
            continue;
        }

        const asset::AssetMetadata& meta = content_opt->metadata;
        const bool is_new = meta.sub_assets.IsEmpty();
        const bool is_dirty = !is_new && IsAssetDirty(file_path, meta);

        if (is_new || is_dirty)
        {
            // Background Cook 목록에 Push
            dirty_vpaths.Push(*file_vpath);
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

        RegisterFromMeta(*file_vpath, meta);
    }

    // 백그라운드 병렬 Cook
    if (!dirty_vpaths.IsEmpty())
    {
        const usize total_tasks = dirty_vpaths.Len();
        std::atomic<usize> completed = 0;

        ConsoleLog(ELogLevel::Info, "Dispatching {} background cook tasks", total_tasks);

        Array<JobHandle> cook_handles;
        cook_handles.Reserve(total_tasks);

        for (const VPath& vpath : dirty_vpaths)
        {
            cook_handles.Push(JobSystem::Get().SubmitTask(MakeCookTask(*this, vpath, completed, total_tasks)));
        }

        for (const JobHandle& handle : cook_handles)
        {
            handle.Wait();
        }
        ConsoleLog(ELogLevel::Info, "All {} cook tasks completed.", total_tasks);
    }

    // === 삭제된 파일 감지 (Hot Start 전용) ===
    uint32 orphaned_count = 0;
    if (is_hot_start)
    {
        asset::AssetRegistry& registry = asset_subsystem->GetRegistry();

        Array<VPath> orphaned;
        registry.VisitAllPaths([&found_vpaths, &orphaned](const VPath& registered_vpath)
        {
            if (!found_vpaths.Contains(registered_vpath))
            {
                orphaned.Push(registered_vpath);
            }
        });

        for (const VPath& vpath : orphaned)
        {
            ConsoleLog(ELogLevel::Warning, "Asset file deleted (offline): {}", vpath);

            // DependencyGraph에서 먼저 제거 (Registry 삭제 전에 ID 목록 확보)
            for (const asset::AssetId& id : registry.GetAssetsInFile(vpath))
            {
                dep_graph.RemoveNode(id);
            }

            registry.UnregisterByPath(vpath);

            // VPath -> 물리 경로로 변환하여 .meta 삭제
            if (const Path physical = VFS::ToPath(vpath); !physical.IsEmpty())
            {
                MetaFileManager::DeleteMeta(physical);
            }
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

    // === DependencyGraph 초기 구축 ===
    BuildDependencyGraph();
}

Optional<MetaFileContent> EditorAssetSubsystem::EnsureMetaFile(const Path& source_path)
{
    ZoneScopedN("EditorAssetSubsystem::EnsureMetaFile");

    // .meta가 있는지 확인
    if (MetaFileManager::HasMeta(source_path))
    {
        // 있다면 Load
        if (Optional existing_content = MetaFileManager::Load(source_path))
        {
            return existing_content;
        }

        ConsoleLog(ELogLevel::Error, "EnsureMetaFile: .meta exists but failed to load for {}. Aborting to prevent data loss.", source_path);
        return NullOpt;
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

bool EditorAssetSubsystem::CookAsset(const VPath& file_vpath)
{
    ZoneScopedN("EditorAssetSubsystem::CookAsset");

    // 이중 Cook 방지 (병렬 dispatch 또는 DDC miss handler 동시 호출에 대비)
    {
        std::scoped_lock lock{ cooking_mutex };
        if (currently_cooking.Contains(file_vpath))
        {
            ConsoleLog(ELogLevel::Debug, "CookAsset: Already cooking, skipping: {}", file_vpath);
            return false;
        }
        currently_cooking.Insert(file_vpath);
    }
    SE_SCOPE_DEFER {
        std::scoped_lock lock{ cooking_mutex };
        currently_cooking.Remove(file_vpath);
    };

    // VPath -> 물리 경로 변환 (파일 I/O에 필요)
    const Optional physical_opt = VFS::Resolve(file_vpath);
    if (!physical_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "CookAsset: Failed to resolve VPath: {}", file_vpath);
        return false;
    }
    const Path& file_path = *physical_opt;

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

            if (!Implements<IPipelineProcessor>(info_opt->type_id))
            {
                ConsoleLog(
                    ELogLevel::Warning,
                    "CookAsset: Processor type does not implement IPipelineProcessor (Skipping): {}",
                    entry.processor_type.GetName()
                );
                continue;
            }

            void* raw = info_opt->constructor();
            IPipelineProcessor* processor = CastFromRaw<IPipelineProcessor>(raw, info_opt->type_id);

            stack.AddProcessor(std::unique_ptr<IPipelineProcessor>(processor));
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

    // 기존 sub-asset의 dependencies를 이름 기준으로 임시 보존
    // (Translator가 per-sub-asset deps를 직접 산출할 때까지 .meta에 저장된 값을 유지)
    HashMap<String, Array<asset::AssetDependencyEntry>> prev_sub_deps;
    for (const asset::SubAssetMeta& old_sub : updated_content.metadata.sub_assets)
    {
        if (!old_sub.dependencies.IsEmpty())
        {
            prev_sub_deps.Insert(old_sub.name, old_sub.dependencies);
        }
    }
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
        asset::AssetPath asset_path = asset::AssetPath{ file_vpath, name };

        // 기존 ID 재사용 또는 새 GUID 발급
        asset::AssetId asset_id = registry.GetAssetId(asset_path).ValueOr(asset::AssetId{ Guid::NewGuid() });

        // Meta에 Sub-asset 정보 추가
        // TODO: Translator에서 참조 텍스처/머티리얼 등을 분석하여
        //       per-sub-asset dependencies를 산출하도록 확장 필요
        updated_content.metadata.sub_assets.Push({
            .name = name,
            .guid = asset_id.GetGuid(),
            .type = asset_type,
            // Translator가 deps를 산출할 때까지 기존 .meta 값을 보존
            .dependencies = prev_sub_deps.Find(name).Copy().ValueOrDefault(),
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

        // AssetPool 등록은 Callback 후 자동으로 이루어짐
        // [AssetSubsystem::LoadInternal 참고]
        // TODO: Hot-reload 시 AssetPool::FindOrCreate + ExchangeAsset으로 메모리 교체
    }

    // DependencyGraph 동기화 (각 sub-asset의 개별 의존성 사용)
    for (const asset::SubAssetMeta& sub : updated_content.metadata.sub_assets)
    {
        SyncDependencies(asset::AssetId{ sub.guid }, sub.dependencies);
    }

    // .meta 파일 갱신 (Sub-asset 정보 기록)
    if (!MetaFileManager::Save(file_path, updated_content))
    {
        ConsoleLog(ELogLevel::Warning, "CookAsset: Failed to update .meta for: {}", file_path);
    }

    ConsoleLog(ELogLevel::Debug, "Successfully cooked {} assets from: {}", result.GetCount(), file_path);
    return true;
}

bool EditorAssetSubsystem::ImportExternalFile(const Path& source_path)
{
    ZoneScopedN("EditorAssetSubsystem::ImportExternalFile");

    // Import 가능한 파일인지 확인
    if (!importer->CanImport(source_path))
    {
        ConsoleLog(ELogLevel::Warning, "ImportExternalFile: Unsupported file type: {}", source_path);
        return false;
    }

    // 첫 번째 스캔 스킴의 물리 경로를 드롭 대상 디렉토리로 사용
    Path content_dir;
    AssetScanSettings scan_settings;
    if (auto config_result = ConfigFile::Load("Config://EditorConfig.toml"))
    {
        scan_settings = config_result->GetSection<AssetScanSettings>("asset_scan");
    }

    if (!scan_settings.schemes.IsEmpty())
    {
        const String& first_scheme = scan_settings.schemes.Front().Value();
        content_dir = VFS::ToPath(VPath{ String::Format("{}://", first_scheme) });
    }

    if (content_dir.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "ImportExternalFile: No content directory configured");
        return false;
    }

    // 파일을 Content 디렉토리로 복사
    const Optional<String> filename = source_path.FileName();
    if (!filename)
    {
        ConsoleLog(ELogLevel::Error, "ImportExternalFile: Cannot extract filename from: {}", source_path);
        return false;
    }
    const Path dest_path = content_dir / *filename;
    if (dest_path.Exists())
    {
        ConsoleLog(ELogLevel::Warning, "ImportExternalFile: File already exists, overwriting: {}", dest_path);
    }

    if (!FileSystem::Copy(source_path, dest_path))
    {
        ConsoleLog(ELogLevel::Error, "ImportExternalFile: Failed to copy {} -> {}", source_path, dest_path);
        return false;
    }

    // .meta 생성 + Cook
    if (!EnsureMetaFile(dest_path))
    {
        ConsoleLog(ELogLevel::Error, "ImportExternalFile: Failed to create .meta for: {}", dest_path);
        return false;
    }

    const Optional file_vpath = VFS::Unresolve(dest_path);
    if (!file_vpath)
    {
        ConsoleLog(ELogLevel::Error, "ImportExternalFile: Failed to resolve VPath for: {}", dest_path);
        return false;
    }

    const bool cook_success = CookAsset(*file_vpath);
    if (cook_success)
    {
        ConsoleLog(ELogLevel::Info, "ImportExternalFile: Successfully imported: {}", *file_vpath);
    }
    return cook_success;
}

void EditorAssetSubsystem::RegisterFromMeta(const VPath& source_vpath, const asset::AssetMetadata& meta)
{
    ZoneScopedN("EditorAssetSubsystem::RegisterFromMeta");

    asset::AssetRegistry& registry = asset_subsystem->GetRegistry();

    // Sub-asset 목록이 비어있으면 아직 Import가 안 된 상태이므로 스킵
    if (meta.sub_assets.IsEmpty())
    {
        return;
    }

    // 각 Sub-asset을 Registry에 등록
    // TODO(memory): 현재 sub_meta = meta를 sub-asset마다 복사하므로, N개의 sub-asset이 있으면
    //   sub_assets[] 배열이 N번 복제됨 (O(N²) 메모리). AssetRecord를 shared_ptr<AssetFileMetadata>로
    //   분리하면 파일 수준 데이터를 한 번만 저장할 수 있음.
    for (const asset::SubAssetMeta& sub : meta.sub_assets)
    {
        const asset::AssetId asset_id{ sub.guid };
        asset::AssetPath asset_path{ source_vpath, sub.name };

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
    const VPath snapshot_vpath = GetRegistrySnapshotVPath();
    const Path snapshot_path = VFS::ToPath(snapshot_vpath);
    if (snapshot_path.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "SaveRegistrySnapshot: Failed to resolve VPath: {}", snapshot_vpath);
        return;
    }

    if (registry.SaveToFile(snapshot_path))
    {
        ConsoleLog(ELogLevel::Info, "Registry snapshot saved: {}", snapshot_vpath);
    }
}

bool EditorAssetSubsystem::LoadRegistrySnapshot()
{
    ZoneScopedN("EditorAssetSubsystem::LoadRegistrySnapshot");

    const VPath snapshot_vpath = GetRegistrySnapshotVPath();
    const Optional snapshot_path = VFS::Resolve(snapshot_vpath);
    if (!snapshot_path.HasValue())
    {
        return false;
    }

    return asset_subsystem->GetRegistry().LoadFromFile(*snapshot_path);
}

VPath EditorAssetSubsystem::GetRegistrySnapshotVPath()
{
    return VPath{ "Cache://registry.bin" };
}

void EditorAssetSubsystem::BuildDependencyGraph()
{
    ZoneScopedN("EditorAssetSubsystem::BuildDependencyGraph");

    const asset::AssetRegistry& registry = asset_subsystem->GetRegistry();
    dep_graph.Clear();

    Array<VPath> all_paths;
    registry.VisitAllPaths([&](const VPath& source_vpath)
    {
        all_paths.Push(source_vpath);
    });

    // Registry에 등록된 모든 소스 파일을 순회하며, 각 sub-asset의 개별 의존성으로 그래프를 구축
    for (const VPath& source_vpath : all_paths)
    {
        const Array<asset::AssetId> assets_in_file = registry.GetAssetsInFile(source_vpath);
        if (assets_in_file.IsEmpty())
        {
            continue;
        }

        // 파일의 SubAssetMeta 목록을 한 번만 읽음 (같은 파일의 모든 레코드가 동일한 sub_assets를 공유)
        Array<asset::SubAssetMeta> sub_assets;
        registry.ReadRecord(assets_in_file.Front().Value(), [&](const asset::AssetRecord& record)
        {
            sub_assets = record.metadata.sub_assets;
        });

        for (const asset::SubAssetMeta& sub : sub_assets)
        {
            if (!sub.dependencies.IsEmpty())
            {
                SyncDependencies(asset::AssetId{ sub.guid }, sub.dependencies);
            }
        }
    }

    ConsoleLog(ELogLevel::Info, "DependencyGraph built: {} nodes", dep_graph.GetNodeCount());
}

void EditorAssetSubsystem::SyncDependencies(
    const asset::AssetId& asset_id,
    ArrayView<const asset::AssetDependencyEntry> dependencies
)
{
    if (dependencies.IsEmpty())
    {
        dep_graph.SetDependencies(asset_id, {});
        return;
    }

    const asset::AssetRegistry& registry = asset_subsystem->GetRegistry();
    Array<asset::AssetId> resolved_ids;
    resolved_ids.Reserve(dependencies.Len());

    for (const asset::AssetDependencyEntry& entry : dependencies)
    {
        // 1. 명시적 GUID가 있으면 직접 사용
        if (entry.asset_guid.IsValid())
        {
            resolved_ids.Push(asset::AssetId{ entry.asset_guid });
            continue;
        }

        // 2. VPath로 Registry에서 파일 내 에셋 ID를 찾음
        // asset_guid가 비어있으면 "파일 전체에 의존" - 파일 내 모든 sub-asset을 등록
        // TODO: entry.type (Hard/Soft/BuildOnly) 미반영 - 현재 모든 의존성을 Hard로 취급
        //       DependencyGraph가 type을 저장하도록 확장 시 반영 예정
        if (!entry.source_vpath.IsEmpty())
        {
            const VPath dep_vpath{ entry.source_vpath };
            const Array<asset::AssetId> dep_assets = registry.GetAssetsInFile(dep_vpath);
            if (!dep_assets.IsEmpty())
            {
                for (const asset::AssetId& dep_id : dep_assets)
                {
                    resolved_ids.Push(dep_id);
                }
            }
            else
            {
                ConsoleLog(
                    ELogLevel::Warning,
                    "SyncDependencies: Unresolved VPath dependency '{}' for asset '{}'",
                    entry.source_vpath, asset_id.GetGuid()
                );
            }
        }
        else
        {
            ConsoleLog(
                ELogLevel::Warning,
                "SyncDependencies: Dependency entry has no GUID and no VPath for asset '{}'", asset_id.GetGuid()
            );
        }
    }

    // 순환 의존성 검사 및 자기 참조 필터링은 SetDependencies 내부에서
    // unique_lock 하에 원자적으로 수행됨 (TOCTOU 방지)
    dep_graph.SetDependencies(asset_id, resolved_ids);
}
} // namespace se::editor
