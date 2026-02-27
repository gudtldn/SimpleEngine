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

    Path file_path = asset_path.GetFilePath();
    records.Insert(asset_id, {
        .id = asset_id,
        .type = asset_type,
        .logical_path = asset_path,
        .metadata = std::move(meta),
    });

    path_to_id.Insert(std::move(asset_path), asset_id);
    file_to_assets.Emplace(std::move(file_path)).Push(asset_id);
}

void AssetRegistry::UnregisterAsset(const AssetId& asset_id)
{
    std::unique_lock lock(registry_mutex);

    // records에서 Asset을 찾은 뒤 연쇄적으로 제거
    if (const Optional record_opt = records.Find(asset_id))
    {
        const Path file_path = record_opt->logical_path.GetFilePath();

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

Optional<AssetId> AssetRegistry::FindFirstOfType(const Path& file_path, const TypeId& type) const
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

Array<AssetId> AssetRegistry::GetAssetsInFile(const Path& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return file_to_assets.Find(file_path).Copy().ValueOrDefault();
}

bool AssetRegistry::IsFileImported(const Path& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return file_to_assets.Contains(file_path);
}

uint32 AssetRegistry::GetAssetCount() const
{
    std::shared_lock lock(registry_mutex);
    return static_cast<uint32>(records.Len());
}

/** AssetRegistry 바이너리 파일 매직 넘버 ("SEAR" = SimpleEngine Asset Registry) */
static constexpr uint32 REGISTRY_MAGIC =
    static_cast<uint32>('S')
    | (static_cast<uint32>('E') << 8)
    | (static_cast<uint32>('A') << 16)
    | (static_cast<uint32>('R') << 24);
static constexpr uint32 REGISTRY_VERSION = 1;

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
    writer << const_cast<HashMap<AssetId, AssetRecord>&>(records);

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

    const auto buffer_opt = FileSystem::ReadBytes(file_path);
    if (!buffer_opt.HasValue())
    {
        return false;
    }

    const Array<uint8>& buffer = buffer_opt.Value();
    MemoryReader reader(buffer);

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
        const Path source_file = record.logical_path.GetFilePath();

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
