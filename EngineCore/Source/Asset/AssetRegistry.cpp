#include "SimpleEngine/Asset/AssetRegistry.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"


namespace se::asset
{
SE_BEGIN_REFLECT(AssetRecord, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(id, meta::Property)
    SE_REFLECT_PROPERTY(type, meta::Property)
    SE_REFLECT_PROPERTY(logical_path, meta::Property)
    SE_REFLECT_PROPERTY(metadata, meta::Property)
SE_END_REFLECT(AssetRecord)


void AssetRegistry::RegisterAsset(
    const AssetId& asset_id, const TypeId& asset_type,
    AssetPath asset_path, AssetMetadata meta
)
{
    std::unique_lock lock(registry_mutex);

    // 동일 ID로 재등록 시 이전 경로 인덱스를 정리 (Asset 이동/재스캔 시 stale 인덱스 방지)
    if (const Optional old_record = records.Find(asset_id))
    {
        const VPath old_file = old_record->logical_path.GetFilePath();
        path_to_id.Remove(old_record->logical_path);

        if (const Optional old_entries = file_to_assets.Find(old_file))
        {
            old_entries->RemoveIf([&asset_id](const AssetId& id)
            {
                return id == asset_id;
            });

            if (old_entries->IsEmpty())
            {
                file_to_assets.Remove(old_file);
            }
        }
    }

    records.Insert(asset_id, {
        .id = asset_id,
        .type = asset_type,
        .logical_path = asset_path,
        .metadata = std::move(meta),
    });

    VPath file_path = asset_path.GetFilePath();
    path_to_id.Insert(std::move(asset_path), asset_id);
    file_to_assets.Emplace(std::move(file_path)).Push(asset_id);
}

void AssetRegistry::UnregisterAsset(const AssetId& asset_id)
{
    std::unique_lock lock(registry_mutex);

    // records에서 Asset을 찾은 뒤 연쇄적으로 제거
    if (const Optional record_opt = records.Find(asset_id))
    {
        const VPath file_path = record_opt->logical_path.GetFilePath();

        // AssetPath 인덱스 제거
        path_to_id.Remove(record_opt->logical_path);

        // file_to_assets에서 해당 ID 제거
        if (const Optional entries_opt = file_to_assets.Find(file_path))
        {
            entries_opt->RemoveIf([&asset_id](const AssetId& id)
            {
                return id == asset_id;
            });

            if (entries_opt->IsEmpty())
            {
                file_to_assets.Remove(file_path);
            }
        }
    }

    records.Remove(asset_id);
}

void AssetRegistry::Clear()
{
    std::unique_lock lock(registry_mutex);

    records.Clear();
    path_to_id.Clear();
    file_to_assets.Clear();
}

Optional<AssetId> AssetRegistry::GetAssetId(const AssetPath& asset_path) const
{
    std::shared_lock lock(registry_mutex);
    return path_to_id.Find(asset_path).Copy();
}

Optional<TypeId> AssetRegistry::GetAssetType(const AssetId& asset_id) const
{
    std::shared_lock lock(registry_mutex);
    return records.Find(asset_id).Map([](const AssetRecord& record)
    {
        return record.type;
    });
}

Optional<AssetId> AssetRegistry::FindFirstOfType(const VPath& file_path, const TypeId& type) const
{
    std::shared_lock lock(registry_mutex);

    if (const Optional entries_opt = file_to_assets.Find(file_path))
    {
        for (const AssetId& id : *entries_opt)
        {
            if (const Optional record_opt = records.Find(id))
            {
                if (record_opt->type == type)
                {
                    return id;
                }
            }
        }
    }
    return NullOpt;
}

Array<AssetId> AssetRegistry::GetAssetsInFile(const VPath& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return file_to_assets.Find(file_path).Copy().ValueOrDefault();
}

bool AssetRegistry::IsFileImported(const VPath& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return file_to_assets.Contains(file_path);
}

uint32 AssetRegistry::GetAssetCount() const
{
    std::shared_lock lock(registry_mutex);
    return static_cast<uint32>(records.Len());
}

void AssetRegistry::VisitAllPaths(const Function<void(const VPath&)>& visitor) const
{
    std::shared_lock lock(registry_mutex);
    for (const VPath& path : file_to_assets | std::views::keys)
    {
        visitor(path);
    }
}

void AssetRegistry::UnregisterByPath(const VPath& source_path)
{
    std::unique_lock lock(registry_mutex);

    const Optional entries_opt = file_to_assets.Find(source_path);
    if (!entries_opt.HasValue())
    {
        return;
    }

    // ID 목록을 복사 (순회 중 삭제 방지)
    const Array<AssetId> ids_to_remove = *entries_opt;

    for (const AssetId& id : ids_to_remove)
    {
        if (const Optional record_opt = records.Find(id))
        {
            path_to_id.Remove(record_opt->logical_path);
        }
        records.Remove(id);
    }

    file_to_assets.Remove(source_path);
}

/** AssetRegistry 바이너리 파일 매직 넘버 ("SEAR" = SimpleEngine Asset Registry) */
static constexpr uint32 REGISTRY_MAGIC =
    static_cast<uint32>('S')
    | (static_cast<uint32>('E') << 8)
    | (static_cast<uint32>('A') << 16)
    | (static_cast<uint32>('R') << 24);
static constexpr uint32 REGISTRY_VERSION = 2;

bool AssetRegistry::SaveToFile(const Path& file_path) const
{
    ZoneScopedN("AssetRegistry::SaveToFile");

    std::shared_lock lock(registry_mutex);

    Array<uint8> buffer;
    MemoryWriter writer(buffer);

    // 헤더
    uint32 magic = REGISTRY_MAGIC;
    uint32 version = REGISTRY_VERSION;
    writer << magic;
    writer << version;

    // records만 직렬화
    writer << records;

    // 디스크 I/O
    if (!FileSystem::Write(file_path, buffer))
    {
        ConsoleLog(ELogLevel::Error, "AssetRegistry::SaveToFile - Failed to write file: {}", file_path);
        return false;
    }

    ConsoleLog(ELogLevel::Info, "AssetRegistry saved: {} assets -> {}", records.Len(), file_path);
    return true;
}

bool AssetRegistry::LoadFromFile(const Path& file_path)
{
    ZoneScopedN("AssetRegistry::LoadFromFile");

    const FileResult<Array<uint8>> file_result = FileSystem::ReadBytes(file_path);
    if (!file_result.HasValue())
    {
        return false;
    }

    MemoryReader reader{ *file_result };

    // 헤더 검증
    uint32 magic = 0;
    uint32 version = 0;
    reader << magic;
    reader << version;

    if (magic != REGISTRY_MAGIC)
    {
        ConsoleLog(ELogLevel::Error, "AssetRegistry::LoadFromFile - Invalid magic number in: {}", file_path);
        return false;
    }
    if (version != REGISTRY_VERSION)
    {
        ConsoleLog(ELogLevel::Warning, "AssetRegistry::LoadFromFile - Version mismatch (expected: {}, got: {})", REGISTRY_VERSION, version);
        return false;
    }

    // 기존 데이터 초기화 후 로드
    std::unique_lock lock(registry_mutex);
    records.Clear();
    path_to_id.Clear();
    file_to_assets.Clear();

    // records 역직렬화
    reader << records;

    // 보조 인덱스 재구축
    for (const auto& [id, record] : records)
    {
        const VPath source_file = record.logical_path.GetFilePath();

        path_to_id.Insert(record.logical_path, id);
        file_to_assets.Emplace(source_file).Push(id);
    }

    for (Array<AssetId>& asset_ids : file_to_assets | std::views::values)
    {
        std::ranges::sort(asset_ids, [this](const AssetId& a, const AssetId& b)
        {
            return records.FindChecked(a).logical_path < records.FindChecked(b).logical_path;
        });
    }

    ConsoleLog(ELogLevel::Info, "AssetRegistry loaded: {} assets from {}", records.Len(), file_path);
    return true;
}
} // namespace se::asset
