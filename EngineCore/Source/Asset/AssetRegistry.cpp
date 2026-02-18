#include "SimpleEngine/Asset/AssetRegistry.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"


namespace se::asset
{
// ---- 데이터 등록 ----

void AssetRegistry::RegisterAsset(const AssetId& asset_id, const TypeId& asset_type, AssetPath&& asset_path)
{
    std::unique_lock lock(registry_mutex);

    Path file_path = asset_path.GetFilePath();

    path_to_id.Insert(asset_path, asset_id);
    id_to_path.Insert(asset_id, std::move(asset_path));
    id_to_type.Insert(asset_id, asset_type);

    file_to_assets.Emplace(std::move(file_path)).Push({
        .id = asset_id,
        .type = asset_type,
    });
}

void AssetRegistry::RegisterMeta(const AssetId& asset_id, AssetMetadata&& meta)
{
    std::unique_lock lock(registry_mutex);
    id_to_meta.Insert(asset_id, std::move(meta));
}

void AssetRegistry::UnregisterAsset(const AssetId& asset_id)
{
    std::unique_lock lock(registry_mutex);

    // id_to_path에서 경로를 먼저 꺼내야 path_to_id에서도 제거 가능
    if (const Optional path_opt = id_to_path.Find(asset_id))
    {
        const Path file_path = path_opt->GetFilePath();

        path_to_id.Remove(*path_opt);

        // file_to_assets에서 해당 엔트리 제거
        if (const Optional entries_opt = file_to_assets.Find(file_path))
        {
            Array<AssetEntry>& entries = const_cast<Array<AssetEntry>&>(*entries_opt);
            entries.RemoveIf([&asset_id](const AssetEntry& e) { return e.id == asset_id; });

            if (entries.IsEmpty())
            {
                file_to_assets.Remove(file_path);
                imported_files.Remove(file_path);
            }
        }
    }

    id_to_path.Remove(asset_id);
    id_to_type.Remove(asset_id);
    id_to_meta.Remove(asset_id);
}

void AssetRegistry::Clear()
{
    std::unique_lock lock(registry_mutex);

    imported_files.Clear();
    path_to_id.Clear();
    id_to_path.Clear();
    id_to_type.Clear();
    file_to_assets.Clear();
    id_to_meta.Clear();
}

// ---- 조회 ----

Optional<const AssetId&> AssetRegistry::GetAssetId(const AssetPath& asset_path) const
{
    std::shared_lock lock(registry_mutex);
    return path_to_id.Find(asset_path);
}

Optional<const AssetPath&> AssetRegistry::GetAssetPath(const AssetId& asset_id) const
{
    std::shared_lock lock(registry_mutex);
    return id_to_path.Find(asset_id);
}

Optional<const TypeId&> AssetRegistry::GetAssetType(const AssetId& asset_id) const
{
    std::shared_lock lock(registry_mutex);
    return id_to_type.Find(asset_id);
}

Optional<const AssetId&> AssetRegistry::FindFirstOfType(const Path& file_path, const TypeId& type) const
{
    std::shared_lock lock(registry_mutex);

    if (const Optional entries_opt = file_to_assets.Find(file_path))
    {
        for (const AssetEntry& entry : *entries_opt)
        {
            if (entry.type == type)
            {
                return entry.id;
            }
        }
    }
    return std::nullopt;
}

Optional<const Array<AssetEntry>&> AssetRegistry::GetAssetsInFile(const Path& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return file_to_assets.Find(file_path);
}

Optional<const AssetMetadata&> AssetRegistry::GetMeta(const AssetId& asset_id) const
{
    std::shared_lock lock(registry_mutex);
    return id_to_meta.Find(asset_id);
}

void AssetRegistry::MarkFileAsImported(const Path& file_path)
{
    std::unique_lock lock(registry_mutex);
    imported_files.Insert(file_path);
}

bool AssetRegistry::IsFileImported(const Path& file_path) const
{
    std::shared_lock lock(registry_mutex);
    return imported_files.Contains(file_path);
}

uint32 AssetRegistry::GetAssetCount() const
{
    std::shared_lock lock(registry_mutex);
    return static_cast<uint32>(id_to_path.Len());
}

// ---- 바이너리 역/직렬화 ----

/** 레지스트리 바이너리 파일 매직 넘버 ("SEAR" = SimpleEngine Asset Registry) */
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

    // 에셋 개수
    uint64 asset_count = id_to_path.Len();
    writer << asset_count;

    // 각 에셋 정보 기록
    for (auto& [id, path] : id_to_path)
    {
        AssetId id_copy = id;
        writer << id_copy;

        AssetPath path_copy = path;
        writer << path_copy;

        TypeId type_id;
        if (const Optional type_opt = id_to_type.Find(id))
        {
            type_id = *type_opt;
        }
        writer << type_id;

        // Metadata
        bool has_meta = id_to_meta.Contains(id);
        writer << has_meta;

        if (has_meta)
        {
            AssetMetadata meta_copy = *id_to_meta.Find(id);
            writer << meta_copy;
        }
    }

    // 파일에 쓰기
    if (!FileSystem::Write(file_path, buffer))
    {
        ConsoleLog(ELogLevel::Error, "AssetRegistry::SaveToFile - Failed to write file: {}", file_path);
        return false;
    }

    ConsoleLog(ELogLevel::Info, "AssetRegistry saved: {} assets -> {}", asset_count, file_path);
    return true;
}

bool AssetRegistry::LoadFromFile(const Path& file_path)
{
    ZoneScopedN("AssetRegistry::LoadFromFile");

    const Optional buffer_opt = FileSystem::Read(file_path);
    if (!buffer_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "AssetRegistry::LoadFromFile - File not found: {}", file_path);
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
    imported_files.Clear();
    path_to_id.Clear();
    id_to_path.Clear();
    id_to_type.Clear();
    file_to_assets.Clear();
    id_to_meta.Clear();

    uint64 asset_count = 0;
    reader << asset_count;

    for (uint64 i = 0; i < asset_count; ++i)
    {
        AssetId asset_id;
        reader << asset_id;

        AssetPath asset_path;
        reader << asset_path;

        TypeId type_id;
        reader << type_id;

        const Path source_file = asset_path.GetFilePath();

        path_to_id.Insert(asset_path, asset_id);
        id_to_path.Insert(asset_id, std::move(asset_path));
        id_to_type.Insert(asset_id, type_id);

        file_to_assets.Emplace(std::move(source_file)).Push({
            .id = asset_id,
            .type = type_id,
        });

        bool has_meta = false;
        reader << has_meta;

        if (has_meta)
        {
            AssetMetadata meta;
            reader << meta;
            id_to_meta.Insert(asset_id, std::move(meta));
        }
    }

    ConsoleLog(ELogLevel::Info, "AssetRegistry loaded: {} assets from {}", asset_count, file_path);
    return true;
}
} // namespace se::asset
