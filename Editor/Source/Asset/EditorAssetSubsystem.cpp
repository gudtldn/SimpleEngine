#include "Asset/EditorAssetSubsystem.h"

#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"
#include "SimpleEditor/Asset/Pipeline/Factories/StaticMeshFactory.h"
#include "SimpleEditor/Asset/Pipeline/Translators/AssimpTranslator.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/SHA256.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


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

    // TODO: 캐시 불러오는 로직
    return true;
}

void EditorAssetSubsystem::Release()
{
    if (asset_subsystem)
    {
        asset_subsystem->SetDDCMissHandler(nullptr);
    }
    importer.reset();

    // TODO: 캐시 저장 로직
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool EditorAssetSubsystem::CookAsset(const Path& file_path)
{
    ZoneScopedN("EditorAssetSubsystem::CookAsset");

    // Import 수행
    const auto result_exp = importer->Import(file_path);
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
    const uint64 file_size = FileSystem::FileSize(file_path).ValueOrDefault();

    auto& registry = asset_subsystem->GetRegistry();
    auto& ddc = asset_subsystem->GetDDC();

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

        // Meta 생성
        asset::AssetMetadata meta;
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

        // Registry 등록
        registry.RegisterAsset(asset_id, asset_type, asset_path, std::move(meta));

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
    }

    ConsoleLog(ELogLevel::Info, "Successfully cooked {} assets from: {}", result.GetCount(), file_path);
    return true;
}
} // namespace se::editor
