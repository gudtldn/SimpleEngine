#include "SimpleEngine/Asset/AssetSubsystem.h"

#include "SimpleEngine/Asset/AssetCache.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEngine/Asset/Pipeline/Translators/AssimpTranslator.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SHA256.h"


namespace se::asset
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

SE_BEGIN_REFLECT(AssetSubsystem, meta::Internal)
SE_END_REFLECT(AssetSubsystem)

AssetSubsystem::AssetSubsystem() = default;
AssetSubsystem::~AssetSubsystem() = default;

bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");

    {
        // Create AssetImporter Instance
        importer = std::make_unique<AssetImporter>();

        // Register Translator
        importer->RegisterTranslator<AssimpTranslator>();

        // Register Factory
        importer->RegisterFactory<StaticMeshFactory>();
    }

    // Create AssetCache Instance
    cache = std::make_unique<AssetCache>();

    // Create Asset Registry Instance
    registry = std::make_unique<AssetRegistry>();

    // Create DerivedDataCache Instance
    ddc = std::make_unique<DerivedDataCache>("DDC");

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");

    ddc.reset();
    registry.reset();
    cache.reset();
    importer.reset();
}

void AssetSubsystem::SetDDCMissHandler(DDCMissHandler handler)
{
    ddc_miss_handler = std::move(handler);
}

void AssetSubsystem::DeferRelease(std::shared_ptr<AssetBase> asset)
{
    // if (asset.use_count() > 2) {} TODO: 검사 할까?
    std::scoped_lock lock(pending_mutex);
    pending_release.Push(std::move(asset));
}

void AssetSubsystem::EndFrame()
{
    ZoneScopedN("AssetSubsystem::EndFrame");

    std::scoped_lock lock(pending_mutex);
    pending_release.Clear();
}

Array<uint8> AssetSubsystem::SerializeAssetPayload(const AssetBase& asset)
{
    const TypeId type_id = asset.GetTypeId();
    const Optional info_opt = TypeRegistry::Get().Find(type_id);
    if (!info_opt || !info_opt->serialize)
    {
        ConsoleLog(ELogLevel::Warning, "Cannot serialize asset type: {}", type_id.GetName());
        return {};
    }

    Array<uint8> payload;
    MemoryWriter writer(payload);
    info_opt->serialize(writer, const_cast<void*>(static_cast<const void*>(&asset)));
    return payload;
}

std::shared_ptr<AssetBase> AssetSubsystem::DeserializeAssetPayload(const TypeId& type_id, const Array<uint8>& payload)
{
    const auto info_opt = TypeRegistry::Get().Find(type_id);
    if (!info_opt || !info_opt->constructor || !info_opt->serialize)
    {
        ConsoleLog(ELogLevel::Warning, "Cannot deserialize asset type: {}", type_id.GetName());
        return nullptr;
    }

    void* raw = info_opt->constructor();
    if (!raw)
    {
        return nullptr;
    }

    MemoryReader reader(payload);
    info_opt->serialize(reader, raw);

    // shared_ptr로 변환 (destructor 연결)
    auto* asset = static_cast<AssetBase*>(raw);
    const auto destructor = info_opt->destructor;
    return std::shared_ptr<AssetBase>(asset, [destructor](AssetBase* p) { destructor(p); });
}

std::shared_ptr<AssetSlot> AssetSubsystem::LoadInternal(const TypeId& expected_type, const AssetPath& source_path)
{
    ZoneScopedN("AssetSubsystem::LoadInternal");
    {
        const String zone_text = String::Format("{} | {}", expected_type.GetName(), source_path.ToString());
        ZoneText(zone_text.CStr(), zone_text.ByteLen());
    }

    Path file_path = source_path.GetFilePath();
    const bool has_sub_name = source_path.HasSubAsset();

    // Registry에서 AssetId 조회
    auto find_asset_id = [&] -> Optional<AssetId>
    {
        if (has_sub_name)
        {
            // Case A: sub_name 있음 -> 정확히 매칭
            return registry->GetAssetId(source_path);
        }

        // Case B: sub_name 없음 -> 해당 타입의 첫 번째 Asset
        return registry->FindFirstOfType(file_path, expected_type);
    };

    // 이미 등록된 Asset인지 확인
    if (const Optional id_opt = find_asset_id())
    {
        const AssetId& current_id = id_opt.Value();

        // 1) 메모리 Cache Hit
        if (auto slot = FindInternal(expected_type, current_id))
        {
            return slot;
        }

        // 2) DDC Hit: Meta가 존재하고, DDC가 유효하면 역직렬화
        String source_hash;
        uint32 cache_version;
        const bool has_meta = registry->ReadRecord(current_id, [&source_hash, &cache_version](const AssetRecord& record)
        {
            source_hash = record.metadata.source_hash;
            cache_version = record.metadata.cache_version;
        });

        if (has_meta)
        {
            if (ddc->IsValid(current_id.GetGuid(), source_hash, cache_version))
            {
                if (auto entry_opt = ddc->Load(current_id.GetGuid()))
                {
                    if (auto asset_ptr = DeserializeAssetPayload(expected_type, entry_opt->payload))
                    {
                        std::shared_ptr<AssetSlot> loaded_slot = cache->FindOrCreate(current_id, expected_type, source_path);
                        if (auto old_asset = loaded_slot->ExchangeAsset(std::move(asset_ptr)))
                        {
                            DeferRelease(std::move(old_asset));
                        }

                        ConsoleLog(ELogLevel::Debug, "Loaded from DDC: {}", source_path.ToString());
                        return loaded_slot;
                    }
                }
            }
        }

        // 여기까지 왔다면 DDC가 없거나 손상된 것.
        ConsoleLog(ELogLevel::Warning, "DDC entry missing or corrupted for: {}", source_path.ToString());
    }

    // TODO: ImportAndRegisterAll 대신 ddc_miss_handler를 사용해서 개선

    // 아직 Import가 안 됐다면, Import 수행
    if (!registry->IsFileImported(file_path))
    {
        if (!ImportAndRegisterAll(file_path))
        {
            return nullptr;
        }
    }

    // 다시 Cache에서 조회
    if (const Optional id_opt = find_asset_id())
    {
        if (auto slot = FindInternal(expected_type, *id_opt))
        {
            return slot;
        }
    }

    // 실패 처리
    if (has_sub_name)
    {
        ConsoleLog(ELogLevel::Error, "Sub-asset not found: {}", source_path.ToString());
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "No asset of type '{}' found in file: {}", expected_type.GetName(), file_path);
    }
    return nullptr;
}

std::shared_ptr<AssetSlot> AssetSubsystem::FindInternal(const TypeId& expected_type, const AssetId& asset_id) const
{
    if (auto slot = cache->Find(asset_id))
    {
        if (slot->GetAssetType() == expected_type)
        {
            return slot;
        }
        ConsoleLog(
            ELogLevel::Error, "Asset Type Mismatch! Requested: {}, Found: {}",
            expected_type.GetName(), slot->GetAssetType().GetName()
        );
    }
    return nullptr;
}


// DEPRECATED
bool AssetSubsystem::ImportAndRegisterAll(const Path& file_path)
{
    ZoneScopedN("AssetSubsystem::ImportAndRegisterAll");

    // Import 수행
    const auto result_exp = importer->Import(file_path);
    if (!result_exp.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Import failed: {}", result_exp.Error().What());
        return false;
    }
    const ImportResult& result = result_exp.Value();

    // 소스 파일 해시 계산 (DDC 저장용)
    const String source_hash = SHA256::HashFile(file_path);
    constexpr uint32 current_cache_version = 1;
    const uint64 file_mtime = FileSystem::LastWriteTime(file_path).ValueOrDefault();
    const uint64 file_size = FileSystem::FileSize(file_path).ValueOrDefault();

    // 모든 Asset을 Registry + Cache에 등록하고 DDC에 저장
    for (const auto& [name, idx] : result.GetNameToIndexMap())
    {
        std::shared_ptr<AssetBase> asset = result.GetAsset(idx);
        if (!asset)
        {
            continue;
        }

        const TypeId asset_type = asset->GetTypeId();
        AssetPath asset_path = AssetPath{ file_path, name };

        // Registry에 이미 등록된 AssetId가 있으면 재사용, 없으면 새로 생성
        AssetId asset_id = [&]
        {
            if (const auto existing = registry->GetAssetId(asset_path))
            {
                return *existing;
            }
            return AssetId{ Guid::NewGuid() };
        }();

        // ---------------------------------------------------------
        // [임시 조치] 바뀐 Registry API 스펙에 맞추기 위한 임시 Meta 생성
        // ---------------------------------------------------------
        AssetMetadata meta;
        meta.guid = asset_id.GetGuid();
        meta.source_hash = source_hash;
        meta.source_mtime = file_mtime;
        meta.source_size = file_size;
        meta.cache_version = current_cache_version;

        meta.sub_assets.Push({
            .name = name,
            .guid = asset_id.GetGuid(),
            .type = asset_type
        });

        // Registry에 등록 (Atomic하게 한 번에 밀어넣음)
        registry->RegisterAsset(asset_id, asset_type, asset_path, std::move(meta));

        // DDC에 저장
        if (!source_hash.IsEmpty())
        {
            Array<uint8> payload = SerializeAssetPayload(*asset);

            if (!payload.IsEmpty())
            {
                CacheEntry entry;
                entry.source_hash = source_hash;
                entry.cache_version = current_cache_version;
                entry.payload = std::move(payload);
                ddc->Store(asset_id.GetGuid(), std::move(entry));
            }
        }

        // Cache에 등록
        const auto slot = cache->FindOrCreate(asset_id, asset_type, AssetPath{ file_path, "" });
        if (auto old_asset = slot->ExchangeAsset(std::move(asset)))
        {
            DeferRelease(std::move(old_asset));
        }
    }

    ConsoleLog(ELogLevel::Debug, "Imported {} assets from: {}", result.GetCount(), file_path);
    return true;
}
} // namespace se::asset
