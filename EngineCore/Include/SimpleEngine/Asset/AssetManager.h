#pragma once
#include <concepts>
#include <memory>
#include <mutex>
#include <utility>

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/Loaders/IAssetLoader.h"
#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/Awaitables.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Utility/Debug.h"
#include "SimpleEngine/Utility/PathResolver.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
/**
 * Asset의 로딩 상태
 */
enum class ELoadingState : uint8
{
    NotLoaded,
    Loading,
    Loaded,
    Failed,
};

struct AssetSlot
{
    std::shared_ptr<IAsset> asset = nullptr;
    ELoadingState state = ELoadingState::NotLoaded;

    // 여러 스레드가 하나의 로딩 완료 이벤트를 기다릴 수 있도록 shared_ptr로 관리
    std::shared_ptr<concurrency::EventWaitHandle> load_event = nullptr;
};

/**
 * 엔진의 모든 에셋 로딩과 생명 주기를 관리합니다.
 */
class SE_CORE_API AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

public:
    // TODO: 추후에 registry 위치 변경
    [[nodiscard]] AssetRegistry& GetRegistry() noexcept { return registry; }

    template <typename AssetType, typename LoaderType>
        requires std::derived_from<AssetType, IAsset> && std::derived_from<LoaderType, IAssetLoader>
    void RegisterLoader(const StringName& extension);

    /**
     * 에셋을 비동기적으로 로드하고, 완료되면 메인 스레드에서 콜백을 호출합니다.
     * @tparam T 로드할 에셋 타입
     * @tparam Fn void(std::shared_ptr<T>) 형태의 콜백 함수
     * @param in_handle 가져오려는 에셋의 핸들
     * @param on_loaded 로딩 완료 시 호출될 콜백 함수. 로딩 실패 시 nullptr가 전달됩니다.
     */
    template <typename T, typename Fn>
        requires std::derived_from<T, IAsset>
        && std::invocable<Fn, std::shared_ptr<T>>
    void LoadAsync(const AssetHandle<T>& in_handle, Fn&& on_loaded);

    /**
     * 에셋을 동기 형태로 가져옵니다. (에셋을 불러오는 동안 Thread가 Blocking됩니다!)
     * @tparam T 가져오려는 에셋 타입
     * @param in_handle 가져오려는 에셋의 핸들
     * @return 불러온 에셋
     */
    template <typename T>
        requires std::derived_from<T, IAsset>
    std::shared_ptr<T> LoadSynchronous(const AssetHandle<T>& in_handle);

public:
    [[nodiscard]] IAssetLoader* GetLoaderForType(const refl::TypeId& type_id) const;
    [[nodiscard]] Optional<const refl::TypeId&> GetTypeFromExtension(const std::filesystem::path& extension) const;
    [[nodiscard]] Optional<const refl::TypeId&> GetTypeFromExtension(const StringName& extension) const;

private:
    template <typename T>
    concurrency::Task<std::shared_ptr<T>> LoadInternal(const Guid& in_guid);

private:
    AssetRegistry registry;

    // 로드된 에셋의 중앙 캐시
    TracyLockable(std::mutex, slots_mutex);
    HashMap<Guid, AssetSlot> asset_slots;

    HashMap<refl::TypeId, std::unique_ptr<IAssetLoader>> loaders;
    HashMap<StringName, refl::TypeId> extension_to_type_map;
};

template <typename AssetType, typename LoaderType>
    requires std::derived_from<AssetType, IAsset> && std::derived_from<LoaderType, IAssetLoader>
void AssetManager::RegisterLoader(const StringName& extension)
{
    const refl::TypeId type_id = refl::TypeId::Get<AssetType>();
    extension_to_type_map.Emplace(extension, type_id);
    loaders.Entry(type_id).OrInsert(std::make_unique<LoaderType>());
}

template <typename T, typename Fn>
    requires std::derived_from<T, IAsset>
    && std::invocable<Fn, std::shared_ptr<T>>
void AssetManager::LoadAsync(const AssetHandle<T>& in_handle, Fn&& on_loaded)
{
    using namespace concurrency;

    TaskScheduler::Get().Launch_IOThread(
        [](AssetManager* self, Guid guid, core::Function<void(std::shared_ptr<T>)> callback) -> Task<void>
        {
            std::shared_ptr<T> asset = co_await self->LoadInternal<T>(guid);

            co_await SwitchToMainThread{};
            if (callback)
            {
                callback(std::move(asset));
            }
        }(this, in_handle.GetGuid(), std::forward<Fn>(on_loaded))
    );
}

template <typename T>
    requires std::derived_from<T, IAsset>
std::shared_ptr<T> AssetManager::LoadSynchronous(const AssetHandle<T>& in_handle)
{
    return concurrency::TaskScheduler::Get().BlockOn(LoadInternal<T>(in_handle.GetGuid()));
}

template <typename T>
concurrency::Task<std::shared_ptr<T>> AssetManager::LoadInternal(const Guid& in_guid)
{
    if (!in_guid.IsValid())
    {
        ConsoleLog(ELogLevel::Warning, "Invalid asset GUID: {}", in_guid.ToString());
        co_return nullptr;
    }

    // Slot 확인
    std::shared_ptr<concurrency::EventWaitHandle> event_to_wait;
    {
        std::scoped_lock lock(slots_mutex);
        AssetSlot& slot = asset_slots[in_guid];

        switch (slot.state)
        {
        // 이미 로딩되어 있는 경우
        case ELoadingState::Loaded:
        {
            co_return std::static_pointer_cast<T>(slot.asset);
        }

        // 로딩중인 경우
        case ELoadingState::Loading:
        {
            // 기존에 있던 event를 가져옴
            event_to_wait = slot.load_event;
            break;
        }

        // 로딩이 안된 경우
        case ELoadingState::NotLoaded:
        case ELoadingState::Failed:
        {
            // event를 새로 생성
            slot.state = ELoadingState::Loading;
            slot.load_event = std::make_shared<concurrency::EventWaitHandle>();
            break;
        }

        default:
            SE_UNREACHABLE();
        }
    }

    // event가 설정되면 (다른 스레드가 로딩중이면) 대기
    if (event_to_wait)
    {
        co_await event_to_wait->Wait();

        std::scoped_lock lock(slots_mutex);
        co_return std::static_pointer_cast<T>(asset_slots.FindChecked(in_guid).asset);
    }

    // Registry에서 메타데이터 가져오기
    Optional<const AssetEntry&> entry_opt = registry.GetEntry(in_guid);
    SE_ASSERT(entry_opt, "Asset not found in registry: {}", in_guid.ToString());

    // Type에 맞는 Loader 가져오기
    IAssetLoader* loader = GetLoaderForType(entry_opt->asset_type);
    SE_ASSERT(loader, "No loader registered for asset type: {}", entry_opt->asset_type.GetName());

    // vpath로부터 실제 경로 가져오기
    auto physical_path_opt = utility::PathResolver::Get().Resolve(entry_opt->virtual_path, false);
    SE_ASSERT(physical_path_opt, "Asset path not found: {}", entry_opt->virtual_path);

    // Asset Load 및 Slot에 저장
    std::shared_ptr<IAsset> loaded_asset = co_await loader->Load(std::move(physical_path_opt).Value());
    {
        std::scoped_lock lock(slots_mutex);

        AssetSlot& slot = asset_slots[in_guid];
        slot.asset = loaded_asset;
        slot.state = loaded_asset ? ELoadingState::Loaded : ELoadingState::Failed;

        // 로딩 완료
        slot.load_event->Set();
        slot.load_event.reset();
    }

    co_return std::static_pointer_cast<T>(std::move(loaded_asset));
}
}
