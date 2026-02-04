#include "Asset/AssetSubsystem.h"

#include "Asset/AssetCache.h"
#include "Asset/AssetRegistry.h"
#include "Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "Asset/Pipeline/Translators/AssimpTranslator.h"
#include "Core/Logging/Logging.h"
#include "Core/Subsystem/SubsystemRegistration.h"


namespace se::asset
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

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

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");

    cache.reset();
    importer.reset();
}

void AssetSubsystem::DeferRelease(std::shared_ptr<IAsset> asset)
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

std::shared_ptr<AssetSlot> AssetSubsystem::LoadInternal(const TypeId& expected_type, const AssetPath& source_path)
{
    ZoneScopedN("AssetSubsystem::LoadInternal");
    {
        const String zone_text = String::Format("{} | {}", expected_type.GetName(), source_path.ToString());
        ZoneText(zone_text.CStr(), zone_text.ByteLen());
    }

    const Path& file_path = source_path.GetFilePath();
    const bool has_sub_name = source_path.HasSubAsset();

    // Registry에서 AssetId 조회
    auto find_asset_id = [&] -> Optional<const AssetId&>
    {
        if (has_sub_name)
        {
            // Case A: sub_name 있음 → 정확히 매칭
            return registry->GetAssetId(source_path);
        }

        // Case B: sub_name 없음 → 해당 타입의 첫 번째 Asset
        return registry->FindFirstOfType(file_path, expected_type);
    };

    // 이미 등록된 Asset인지 확인
    if (const Optional id_opt = find_asset_id())
    {
        if (auto slot = FindInternal(expected_type, *id_opt))
        {
            return slot;
        }
    }

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

std::shared_ptr<AssetSlot> AssetSubsystem::FindInternal(const TypeId& expected_type, const AssetId& id) const
{
    if (auto slot = cache->Find(id))
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

    // 모든 Asset을 Registry + Cache에 등록
    for (const auto& [name, idx] : result.name_to_index)
    {
        std::shared_ptr<IAsset> asset = result.GetAsset(idx);
        if (!asset)
        {
            continue;
        }

        AssetId asset_id = AssetId{ Guid::NewGuid() };
        const TypeId asset_type = asset->GetAssetType();
        AssetPath asset_path = AssetPath{ file_path, name };

        // Registry에 등록
        registry->RegisterAsset(asset_id, asset_type, std::move(asset_path));

        // Cache에 등록
        auto slot = cache->FindOrCreate(asset_id, asset_type, file_path);
        if (auto old_asset = slot->ExchangeAsset(std::move(asset)))
        {
            DeferRelease(std::move(old_asset));
        }
    }

    // Import 완료
    registry->MarkFileAsImported(file_path);

    ConsoleLog(ELogLevel::Debug, "Imported {} assets from: {}", result.GetCount(), file_path);
    return true;
}
} // namespace se::asset
