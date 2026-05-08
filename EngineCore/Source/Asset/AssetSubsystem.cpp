#include "SimpleEngine/Asset/AssetSubsystem.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetPool.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/Concurrency/AsyncFileIO.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
SE_REGISTER_SUBSYSTEM(AssetSubsystem);

SE_BEGIN_REFLECT(AssetSubsystem, meta::Internal)
SE_END_REFLECT(AssetSubsystem)

AssetSubsystem::AssetSubsystem() = default;
AssetSubsystem::~AssetSubsystem() = default;

bool AssetSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Asset subsystem...");

    // Create AssetPool Instance
    pool = std::make_unique<AssetPool>();

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

    // Built-In 에셋 등록 (DefaultLit, DefaultLitInstance, White1x1)
    SeedBuiltinAssets(*this);

    return true;
}

void AssetSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Asset subsystem...");

    ddc.reset();
    registry.reset();
    pool.reset();
}

void AssetSubsystem::SetDDCMissHandler(DDCMissHandler handler)
{
    ddc_miss_handler = std::move(handler);
}

void AssetSubsystem::DeferRelease(AssetPayload payload)
{
    if (!payload.ptr)
    {
        return;
    }

    if (!SE_ENSURE(payload.destructor))
    {
        ConsoleLog(ELogLevel::Error, "AssetSubsystem::DeferRelease - Destructor is null for a valid asset pointer! Memory leak occurred.");
        return;
    }

    pool->DeferDestroy(std::move(payload), frame_count);
}

void AssetSubsystem::EndFrame()
{
    ZoneScopedN("AssetSubsystem::EndFrame");

    ++frame_count;
    pool->GetTable().SetCurrentFrame(frame_count);
    pool->ProcessPendingDestroy(frame_count);
    pool->EvictIfOverBudget(frame_count);
}

Array<uint8> AssetSubsystem::SerializeAssetPayload(const AssetBase& asset)
{
    const TypeId type_id = asset.GetTypeId();
    const auto info_opt = TypeRegistry::Get().Find(type_id);
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

AssetPayload AssetSubsystem::DeserializeAssetPayload(const TypeId& type_id, ArrayView<const uint8> payload_view)
{
    const auto info_opt = TypeRegistry::Get().Find(type_id);
    if (!info_opt || !info_opt->constructor || !info_opt->serialize)
    {
        ConsoleLog(ELogLevel::Warning, "Cannot deserialize asset type: {}", type_id.GetName());
        return {};
    }

    void* raw = info_opt->constructor();
    if (!raw)
    {
        return {};
    }

    MemoryReader reader{ payload_view };
    info_opt->serialize(reader, raw);

    return {
        .ptr = static_cast<AssetBase*>(raw),
        .destructor = info_opt->destructor
    };
}

HandleData AssetSubsystem::LoadInternal(const TypeId& expected_type, const AssetPath& source_path, EScopeLayer scope)
{
    ZoneScopedN("AssetSubsystem::LoadInternal");
    SE_DEBUG_EXPRESSION({
        const String zone_text = String::Format("{} | {}", expected_type.GetName(), source_path.ToString());
        ZoneText(zone_text.CStr(), zone_text.ByteLen());
    })

    const VPath& file_vpath = source_path.GetFilePath();
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

    HandleData handle_data;
    HandleTable& table = pool->GetTable();

    // Dangling Reference 방지를 위해, 매번 직접 인덱스로 접근
    const auto get_slot = [&] -> SlotEntry&
    {
        return table.GetSlot(handle_data.index);
    };

    for (usize attempt = 0; attempt < 2; ++attempt)
    {
        bool ddc_missing_or_corrupted = false;

        // 이미 등록된 Asset인지 확인
        if (const auto asset_id = find_asset_id())
        {
            const AssetId& current_id = *asset_id;
            handle_data = pool->FindOrCreate(current_id, expected_type, source_path);

            // [Slot-Level Lock] 로딩 상태 동기화
            switch (AcquireLoadSlot(handle_data, expected_type))
            {
            case ESlotAcquireResult::Loaded:
                return handle_data;
            case ESlotAcquireResult::Failed:
                return {};
            case ESlotAcquireResult::Acquired:
                break;
            default:
                SE_UNREACHABLE();
            }

            // DDC Hit 검사 및 로드 수행
            ContentHash source_hash;
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
                    if (AssetPayload payload = DeserializeAssetPayload(expected_type, entry_opt->payload))
                    {
                        CommitLoadedPayload(handle_data, std::move(payload), entry_opt->payload.Len(), scope);
                        ConsoleLog(ELogLevel::Debug, "Loaded from DDC: {}", source_path.ToString());
                        return handle_data;
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
            if (handle_data.IsValid() && get_slot().GetState() == ELoadingState::Loaded)
            {
                return handle_data;
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
                    if (handle_data.IsValid())
                    {
                        get_slot().SetState(ELoadingState::Unloaded);
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
    if (handle_data.IsValid())
    {
        get_slot().SetState(ELoadingState::Failed);
    }

    if (has_sub_name)
    {
        ConsoleLog(ELogLevel::Error, "Sub-asset not found: {}", source_path.ToString());
    }
    else
    {
        ConsoleLog(ELogLevel::Error, "No asset of type '{}' found in file: {}", expected_type.GetName(), file_vpath);
    }
    return {};
}

// ReSharper disable once CppMemberFunctionMayBeConst
AssetSubsystem::ESlotAcquireResult AssetSubsystem::AcquireLoadSlot(HandleData handle_data, const TypeId& expected_type)
{
    HandleTable& table = pool->GetTable();
    while (true)
    {
        SlotEntry& slot = table.GetSlot(handle_data.index);
        const ELoadingState state = slot.GetState();

        if (state == ELoadingState::Loaded)
        {
            if (slot.asset_type == expected_type)
            {
                return ESlotAcquireResult::Loaded;
            }
            ConsoleLog(ELogLevel::Error, "Asset Type Mismatch!");
            return ESlotAcquireResult::Failed;
        }

        if (state == ELoadingState::Loading)
        {
            // TODO: WaitForLoadComplete는 블로킹 함수라서,
            //       만약 병목이 될 경우 co_await 기반의 비동기 대기(SlotEntry에 awaitable notify)로 교체하여
            //       워커 스레드를 양보할 수 있도록 개선.
            slot.WaitForLoadComplete();
            continue;
        }

        if (slot.BeginLoad())
        {
            return ESlotAcquireResult::Acquired;
        }
    }
}

void AssetSubsystem::CommitLoadedPayload(HandleData handle_data, AssetPayload payload, uint64 payload_size, EScopeLayer scope)
{
    HandleTable& table = pool->GetTable();
    SlotEntry& current_slot = table.GetSlot(handle_data.index);

    AssetPayload old_payload = current_slot.ExchangePayload(std::move(payload));

    const uint64 old_size = current_slot.asset_size_bytes;
    const uint64 new_size = payload_size;
    current_slot.asset_size_bytes = new_size;
    current_slot.last_access_frame.store(frame_count, std::memory_order_relaxed);
    current_slot.scope = scope;

    if (new_size > old_size)
    {
        table.TrackMemoryUsage(new_size - old_size);
    }
    else if (new_size < old_size)
    {
        table.UntrackMemoryUsage(old_size - new_size);
    }

    current_slot.SetState(ELoadingState::Loaded);

    if (old_payload)
    {
        DeferRelease(std::move(old_payload));
    }
}

HandleData AssetSubsystem::RegisterBuiltinInternal(const AssetId& asset_id, const TypeId& type_id, AssetPayload payload, uint64 asset_size)
{
    const HandleData handle_data = pool->FindOrCreate(asset_id, type_id, {});
    SlotEntry& slot = pool->GetTable().GetSlot(handle_data.index);

    if (!slot.BeginLoad())
    {
        if (slot.GetState() == ELoadingState::Loaded)
        {
            return handle_data;
        }
        ConsoleLog(ELogLevel::Warning, "RegisterBuiltinInternal: Slot is not in a loadable state for asset.");
        return {};
    }

    CommitLoadedPayload(handle_data, std::move(payload), asset_size, EScopeLayer::Global);
    return handle_data;
}

HandleData AssetSubsystem::FindInternal(const TypeId& expected_type, const AssetId& asset_id) const
{
    Optional<HandleData> handle_opt = pool->Find(asset_id);
    if (!handle_opt.HasValue())
    {
        return {};
    }

    const HandleData& handle_data = handle_opt.Value();
    const SlotEntry& slot = pool->GetTable().GetSlot(handle_data.index);

    if (slot.asset_type != expected_type)
    {
        ConsoleLog(
            ELogLevel::Error, "Asset Type Mismatch! Requested: {}, Found: {}",
            expected_type.GetName(), slot.asset_type.GetName()
        );
        return {};
    }

    return handle_data;
}

JobTask<HandleData> AssetSubsystem::LoadAsyncInternal(TypeId expected_type, AssetPath source_path, EScopeLayer scope)
{
    // Worker 스레드로 전환 (호출 스레드 비블로킹 보장)
    co_await ResumeOn{ EJobThread::Worker };

    ZoneScopedN("AssetSubsystem::LoadAsyncInternal");
    SE_DEBUG_EXPRESSION({
        const String zone_text = String::Format("{} | {}", expected_type.GetName(), source_path.ToString());
        ZoneText(zone_text.CStr(), zone_text.ByteLen());
    })

    const VPath& file_vpath = source_path.GetFilePath();
    const bool has_sub_name = source_path.HasSubAsset();

    // Registry에서 AssetId 조회
    auto find_asset_id = [&] -> Optional<AssetId>
    {
        if (has_sub_name)
        {
            return registry->GetAssetId(source_path);
        }
        return registry->FindFirstOfType(file_vpath, expected_type);
    };

    const auto id_opt = find_asset_id();
    if (!id_opt)
    {
        // Registry에 없으면 동기 fallback (DDC miss handler가 Import를 수행할 수 있음)
        co_return LoadInternal(expected_type, source_path, scope);
    }

    const AssetId& asset_id = id_opt.Value();
    HandleData handle_data = pool->FindOrCreate(asset_id, expected_type, source_path);

    // 메모리 캐시 hit 또는 다른 스레드가 로딩 중이면 대기
    switch (AcquireLoadSlot(handle_data, expected_type))
    {
    case ESlotAcquireResult::Loaded:
        co_return handle_data;
    case ESlotAcquireResult::Failed:
        co_return HandleData{};
    case ESlotAcquireResult::Acquired:
        break;
    default:
        SE_UNREACHABLE();
    }

    // DDC validity 확인
    ContentHash source_hash;
    uint32 cache_version;
    const bool has_meta = registry->ReadRecord(asset_id, [&source_hash, &cache_version](const AssetRecord& record)
    {
        source_hash = record.metadata.source_hash;
        cache_version = record.metadata.cache_version;
    });

    if (has_meta && ddc->IsValid(asset_id.GetGuid(), source_hash, cache_version))
    {
        // 비동기 I/O로 DDC 캐시 파일 읽기
        const Path cache_path = ddc->BuildCachePath(asset_id.GetGuid());

        if (AsyncFileIO::IsInitialized())
        {
            IOResult io_result = co_await AsyncFileIO::Get().ReadFileAsync(cache_path);

            if (io_result.success && !io_result.data.IsEmpty())
            {
                if (Optional<CacheEntry> entry_opt = DerivedDataCache::ParseFromBuffer(io_result.data))
                {
                    if (AssetPayload payload = DeserializeAssetPayload(expected_type, entry_opt->payload))
                    {
                        CommitLoadedPayload(handle_data, std::move(payload), entry_opt->payload.Len(), scope);
                        ConsoleLog(ELogLevel::Debug, "LoadAsync: Loaded from DDC (async I/O): {}", source_path.ToString());
                        co_return handle_data;
                    }
                }
            }
        }
    }

    // DDC miss 또는 비동기 I/O 실패 -> 동기 fallback
    // BeginLoad 상태를 되돌려서 LoadInternal이 다시 획득할 수 있게 함
    pool->GetTable().GetSlot(handle_data.index).SetState(ELoadingState::Unloaded);

    ConsoleLog(ELogLevel::Debug, "LoadAsync: Falling back to sync LoadInternal: {}", source_path.ToString());
    co_return LoadInternal(expected_type, source_path, scope);
}

HandleTable& AssetSubsystem::GetHandleTable() const
{
    return pool->GetTable();
}
} // namespace se
