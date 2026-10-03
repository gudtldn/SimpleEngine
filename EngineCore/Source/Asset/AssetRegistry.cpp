#include "SimpleEngine/Asset/AssetRegistry.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"


namespace se
{
SE_BEGIN_REFLECT_V1(AssetRecord, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(id, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(type, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(logical_path, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(metadata, meta::Reflect)
SE_END_REFLECT_V1(AssetRecord)


void AssetRegistry::RegisterAsset(
    const AssetId& asset_id, const TypeId_v1& asset_type,
    AssetPath asset_path, AssetMetadata meta
)
{
    std::unique_lock lock(registry_mutex);

    // 동일 ID로 재등록 시 이전 경로 인덱스를 정리 (Asset 이동/재스캔 시 stale 인덱스 방지)
    if (const auto old_record = records.Find(asset_id))
    {
        const VPath old_file = old_record->logical_path.GetFilePath();
        path_to_id.Remove(old_record->logical_path);

        if (const auto old_entries = file_to_assets.Find(old_file))
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
    if (const auto record = records.Find(asset_id))
    {
        const VPath file_path = record->logical_path.GetFilePath();

        // AssetPath 인덱스 제거
        path_to_id.Remove(record->logical_path);

        // file_to_assets에서 해당 ID 제거
        if (const auto entries = file_to_assets.Find(file_path))
        {
            entries->RemoveIf([&asset_id](const AssetId& id)
            {
                return id == asset_id;
            });

            if (entries->IsEmpty())
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

Optional<TypeId_v1> AssetRegistry::GetAssetType(const AssetId& asset_id) const
{
    std::shared_lock lock(registry_mutex);
    return records.Find(asset_id).Map([](const AssetRecord& record)
    {
        return record.type;
    });
}

Optional<AssetId> AssetRegistry::FindFirstOfType(const VPath& file_path, const TypeId_v1& type) const
{
    std::shared_lock lock(registry_mutex);

    if (const auto entries = file_to_assets.Find(file_path))
    {
        for (const AssetId& id : *entries)
        {
            if (const auto record = records.Find(id))
            {
                if (record->type == type)
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

u32 AssetRegistry::GetAssetCount() const
{
    std::shared_lock lock(registry_mutex);
    return static_cast<u32>(records.Len());
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

    const auto entries_opt = file_to_assets.Find(source_path);
    if (!entries_opt.HasValue())
    {
        return;
    }

    // ID 목록을 복사 (순회 중 삭제 방지)
    const Array<AssetId> ids_to_remove = *entries_opt;

    for (const AssetId& id : ids_to_remove)
    {
        if (const auto record = records.Find(id))
        {
            path_to_id.Remove(record->logical_path);
        }
        records.Remove(id);
    }

    file_to_assets.Remove(source_path);
}

bool AssetRegistry::SaveToFile(const Path& file_path) const
{
    ZoneScopedN("AssetRegistry::SaveToFile");

    std::shared_lock lock(registry_mutex);

    // records만 직렬화. 루트 타입과 스키마 해시를 헤더에 남겨, AssetRecord가 바뀌면 옛 스냅샷을 거절하게 함
    const SerializePlan& plan = SerializePlanOf<decltype(records)>();
    Array<u8> buffer;
    BinaryFileWriter writer(buffer, plan.type, plan.SchemaHash());
    const auto result = serde::Serialize(writer, records);
    writer.Finish();
    if (result.HasError())
    {
        ConsoleLog(ELogLevel::Error, "AssetRegistry::SaveToFile - Failed to serialize: {} (path: '{}')", result.Error().message, result.Error().path);
        return false;
    }

    // 디스크 I/O
    if (!fs::Write(file_path, buffer))
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

    const FileResult<Array<u8>> file_result = fs::ReadBytes(file_path);
    if (!file_result.HasValue())
    {
        return false;
    }

    // 예전 형식이나 AssetRecord가 바뀐 스냅샷은 헤더에서, 손상된 스냅샷은 체크섬에서 실패하므로 기존 데이터를 건드리지 않고 false를 반환
    decltype(records) loaded_records;
    const SerializePlan& plan = SerializePlanOf<decltype(records)>();
    BinaryFileReader reader(*file_result, plan.type, plan.SchemaHash());
    if (const auto result = serde::Deserialize(reader, loaded_records); result.HasError())
    {
        ConsoleLog(ELogLevel::Warning, "AssetRegistry::LoadFromFile - Rejected snapshot {}: {}", file_path, result.Error().message);
        return false;
    }

    // 기존 데이터를 교체
    std::unique_lock lock(registry_mutex);
    records = std::move(loaded_records);
    path_to_id.Clear();
    file_to_assets.Clear();

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
} // namespace se


SE_REFLECT_BEGIN(se::AssetRecord)
    SE_FIELD(id)
    SE_FIELD(type)
    SE_FIELD(logical_path)
    SE_FIELD(metadata)
SE_REFLECT_END()
