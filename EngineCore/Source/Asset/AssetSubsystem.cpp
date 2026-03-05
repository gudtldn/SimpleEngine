#include "SimpleEngine/Asset/AssetSubsystem.h"

#include "SimpleEngine/Asset/AssetCache.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"


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

    // Create AssetCache Instance
    cache = std::make_unique<AssetCache>();

    // Create Asset Registry Instance
    registry = std::make_unique<AssetRegistry>();

    // Create DerivedDataCache Instance
    const VPath ddc_vpath = "Cache://DDC";
    const Path ddc_path = VFS::ToPath(ddc_vpath);
    if (ddc_path.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "Failed to resolve DDC path: {}", ddc_vpath);
        return false;
    }
    ddc = std::make_unique<DerivedDataCache>(ddc_path);

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");

    ddc.reset();
    registry.reset();
    cache.reset();
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
    AssetBase* asset = static_cast<AssetBase*>(raw);
    return { asset, [destructor = info_opt->destructor](AssetBase* p) { destructor(p); } };
}

std::shared_ptr<AssetSlot> AssetSubsystem::LoadInternal(const TypeId& expected_type, const AssetPath& source_path)
{
    ZoneScopedN("AssetSubsystem::LoadInternal");
    {
        const String zone_text = String::Format("{} | {}", expected_type.GetName(), source_path.ToString());
        ZoneText(zone_text.CStr(), zone_text.ByteLen());
    }

    VPath file_vpath = source_path.GetFilePath();
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
        return registry->FindFirstOfType(file_vpath, expected_type);
    };

    std::shared_ptr<AssetSlot> slot = nullptr;
    for (usize attempt = 0; attempt < 2; ++attempt)
    {
        bool ddc_missing_or_corrupted = false;

        // 이미 등록된 Asset인지 확인
        if (const Optional id_opt = find_asset_id())
        {
            const AssetId& current_id = id_opt.Value();
            slot = cache->FindOrCreate(current_id, expected_type, source_path);

            // [Slot-Level Lock] 로딩 상태 동기화
            while (true)
            {
                const ELoadingState state = slot->GetState();
                if (state == ELoadingState::Loaded)
                {
                    // 메모리 Cache Hit
                    if (slot->GetAssetType() == expected_type)
                    {
                        return slot;
                    }
                    ConsoleLog(ELogLevel::Error, "Asset Type Mismatch!");
                    return nullptr;
                }

                if (state == ELoadingState::Loading)
                {
                    // 다른 스레드가 DDC를 읽거나 Import 중이므로 Sleep
                    slot->WaitForLoadComplete();
                    continue; // 깨어나면 상태를 다시 확인
                }

                // Unloaded 또는 Failed 라면, 현재 스레드에서 로딩 시작
                if (slot->BeginLoad())
                {
                    break;
                }
            }

            // DDC Hit 검사 및 로드 수행
            String source_hash;
            uint32 cache_version;
            const bool has_meta = registry->ReadRecord(current_id, [&source_hash, &cache_version](const AssetRecord& record)
            {
                source_hash = record.metadata.source_hash;
                cache_version = record.metadata.cache_version;
            });

            if (has_meta && ddc->IsValid(current_id.GetGuid(), source_hash, cache_version))
            {
                if (auto entry_opt = ddc->Load(current_id.GetGuid()))
                {
                    if (auto asset_ptr = DeserializeAssetPayload(expected_type, entry_opt->payload))
                    {
                        if (auto old_asset = slot->ExchangeAsset(std::move(asset_ptr)))
                        {
                            DeferRelease(std::move(old_asset));
                        }
                        ConsoleLog(ELogLevel::Debug, "Loaded from DDC: {}", source_path.ToString());
                        return slot;
                    }
                }
            }

            // 여기까지 왔다면 DDC가 없거나 손상된 것.
            ConsoleLog(ELogLevel::Warning, "DDC entry missing or corrupted for: {}", source_path.ToString());
            ddc_missing_or_corrupted = true;
        }

        /* Import 여부 결정
         *  - 파일 자체가 한 번도 Import 안 된 새 파일이거나 (!IsFileImported)
         *  - 파일은 Import 되었는데 DDC가 날아갔을 때만 (ddc_missing_or_corrupted) Import 수행
         *  (※ 파일이 Import 되었는데 id_opt가 없는 경우는 '없는 에셋(오타 등)'을 찾은 것이므로 건너뜀)
         */
        if (!registry->IsFileImported(file_vpath) || ddc_missing_or_corrupted)
        {
            std::unique_lock lock(loading_mutex);

            // 다른 스레드에서 이미 이 파일을 Import 중이라면 대기
            import_cv.wait(lock, [&] { return !files_currently_importing.Contains(file_vpath); });

            // double-check
            if (slot && slot->GetState() == ELoadingState::Loaded)
            {
                return slot;
            }

            // 아직 Import 안 됨 -> 현재 스레드에서 Import 권한을 획득함
            if (ddc_miss_handler)
            {
                files_currently_importing.Insert(file_vpath);
                lock.unlock(); // 무거운 Import 도중 락 해제

                ConsoleLog(ELogLevel::Info, "Triggering DDC Miss Handler for: {}", file_vpath.ToString());
                const bool import_success = ddc_miss_handler(*this, file_vpath);

                // Import 완료 (성공/실패 무관)
                lock.lock();
                files_currently_importing.Remove(file_vpath);
                import_cv.notify_all(); // 대기 중이던 다른 스레드들을 깨움

                if (import_success)
                {
                    // 성공했다면, 내가 쥐고 있던 Slot의 상태를 Unloaded로 초기화
                    // 그리고 continue 후, 위에서 다시 BeginLoad()를 획득하고 DDC를 읽도록 함.
                    if (slot)
                    {
                        slot->SetState(ELoadingState::Unloaded);
                    }
                    continue;
                }
            }
            else
            {
                ConsoleLog(ELogLevel::Error, "DDC Miss in Runtime mode. Asset not cooked.");
            }
        }

        // 임포트를 할 수 없거나, 임포트에 실패했으면 더 시도할 필요 없이 루프 탈출
        break;
    }

    // 실패 처리
    if (slot)
    {
        slot->SetState(ELoadingState::Failed);
    }

    if (has_sub_name)
    {
        ConsoleLog(ELogLevel::Error, "Sub-asset not found: {}", source_path.ToString());
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "No asset of type '{}' found in file: {}", expected_type.GetName(), file_vpath);
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
} // namespace se::asset
