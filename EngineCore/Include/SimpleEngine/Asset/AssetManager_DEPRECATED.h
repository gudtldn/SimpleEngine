#pragma once
#include <concepts>
#include <memory>
#include <mutex>
#include <utility>

#include "SimpleEngine/Asset/AssetHandle.h"
#include "SimpleEngine/Asset/AssetRegistry_DEPRECATED.h"
#include "SimpleEngine/Asset/ImportSettings_DEPRECATED/DefaultImportSettings.h"
#include "SimpleEngine/Asset/Loaders_DEPRECATED/IAssetLoader.h"
#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/Awaitables.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Utility/Debug.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
/**
 * Asset의 로딩 상태
 */
enum class ELoadingState_DEPRECATED : uint8
{
    NotLoaded,
    Loading,
    Loaded,
    Failed,
};

struct AssetSlot
{
    std::shared_ptr<IAsset> asset = nullptr;
    ELoadingState_DEPRECATED state = ELoadingState_DEPRECATED::NotLoaded;

    // 여러 스레드가 하나의 로딩 완료 이벤트를 기다릴 수 있도록 shared_ptr로 관리
    std::shared_ptr<EventWaitHandle> load_event = nullptr;
};

/**
 * 엔진의 모든 에셋 로딩과 생명 주기를 관리합니다.
 */
class SE_CORE_API AssetManager_DEPRECATED
{
    struct ExtensionInfo;

public:
    AssetManager_DEPRECATED() = default;
    ~AssetManager_DEPRECATED() = default;

    AssetManager_DEPRECATED(const AssetManager_DEPRECATED&) = delete;
    AssetManager_DEPRECATED& operator=(const AssetManager_DEPRECATED&) = delete;
    AssetManager_DEPRECATED(AssetManager_DEPRECATED&&) = delete;
    AssetManager_DEPRECATED& operator=(AssetManager_DEPRECATED&&) = delete;

public:
    // TODO: 추후에 registry 위치 변경
    [[nodiscard]] AssetRegistry_DEPRECATED& GetRegistry() noexcept { return registry; }

    template <typename AssetType, typename LoaderType, typename SettingsType = DefaultImportSettings>
        requires std::derived_from<AssetType, IAsset>
        && std::derived_from<LoaderType, IAssetLoader>
        && std::derived_from<SettingsType, IAssetImportSettings>
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
    [[nodiscard]] Optional<const ExtensionInfo&> GetExtensionInfo(const StringName& extension) const;
    [[nodiscard]] IAssetLoader* GetLoaderFromType(const refl::TypeId& type_id) const;
    [[nodiscard]] std::shared_ptr<IAssetImportSettings> CreateDefaultSettingsForFile(const Path& path) const;
    [[nodiscard]] std::shared_ptr<IAssetImportSettings> CreateSettingsFromType(const refl::TypeId& settings_type) const;

private:
    template <typename T>
    Task<std::shared_ptr<T>> LoadInternal(const Guid& in_guid);

private:
    // 확장자별 등록 정보를 담는 구조체
    struct ExtensionInfo
    {
        refl::TypeId asset_type;    // 생성될 에셋 타입 (예: Texture2D)
        refl::TypeId loader_type;   // 사용할 로더 타입 (예: Texture2DLoader)
        refl::TypeId settings_type; // 사용할 설정 타입 (예: TextureImportSettings)
    };

    HashMap<StringName, ExtensionInfo> extension_registry;

    HashMap<refl::TypeId, std::unique_ptr<IAssetLoader>> loaders;
    HashMap<refl::TypeId, std::shared_ptr<IAssetImportSettings>> settings_prototypes;

private:
    AssetRegistry_DEPRECATED registry;

    // 로드된 에셋의 중앙 캐시
    TracyLockable(std::mutex, slots_mutex);
    HashMap<Guid, AssetSlot> asset_slots;
};

template <typename AssetType, typename LoaderType, typename SettingsType>
    requires std::derived_from<AssetType, IAsset>
    && std::derived_from<LoaderType, IAssetLoader>
    && std::derived_from<SettingsType, IAssetImportSettings>
void AssetManager_DEPRECATED::RegisterLoader(const StringName& extension)
{
    const refl::TypeId asset_type = refl::TypeId::Get<AssetType>();
    const refl::TypeId loader_type = refl::TypeId::Get<LoaderType>();
    const refl::TypeId settings_type = refl::TypeId::Get<SettingsType>();

    // Extension Registry에 통합 정보 저장
    ExtensionInfo info = {
        .asset_type = asset_type,
        .loader_type = loader_type,
        .settings_type = settings_type,
    };
    extension_registry.Emplace(extension, info);

    // Loader 인스턴스 생성 (로더는 타입별로 하나만 있으면 됨)
    loaders.Entry(loader_type).OrInsert(std::make_unique<LoaderType>());

    // Settings Prototypes 등록
    if constexpr (!std::same_as<SettingsType, DefaultImportSettings>)
    {
        settings_prototypes.Entry(settings_type).OrInsert(std::make_shared<SettingsType>());
    }
}

template <typename T, typename Fn>
    requires std::derived_from<T, IAsset>
    && std::invocable<Fn, std::shared_ptr<T>>
void AssetManager_DEPRECATED::LoadAsync(const AssetHandle<T>& in_handle, Fn&& on_loaded)
{
    TaskScheduler::Get().Launch_IOThread(
        [](AssetManager_DEPRECATED* self, Guid guid, Function<void(std::shared_ptr<T>)> callback) -> Task<void>
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
std::shared_ptr<T> AssetManager_DEPRECATED::LoadSynchronous(const AssetHandle<T>& in_handle)
{
    return TaskScheduler::Get().BlockOn(LoadInternal<T>(in_handle.GetGuid()));
}

template <typename T>
Task<std::shared_ptr<T>> AssetManager_DEPRECATED::LoadInternal(const Guid& in_guid)
{
    if (!in_guid.IsValid())
    {
        ConsoleLog(ELogLevel::Warning, "Invalid asset GUID: {}", in_guid.ToString());
        co_return nullptr;
    }

    // Slot 확인
    std::shared_ptr<EventWaitHandle> event_to_wait;
    {
        std::scoped_lock lock(slots_mutex);
        AssetSlot& slot = asset_slots[in_guid];

        switch (slot.state)
        {
        // 이미 로딩되어 있는 경우
        case ELoadingState_DEPRECATED::Loaded:
        {
            co_return std::static_pointer_cast<T>(slot.asset);
        }

        // 로딩중인 경우
        case ELoadingState_DEPRECATED::Loading:
        {
            // 기존에 있던 event를 가져옴
            event_to_wait = slot.load_event;
            break;
        }

        // 로딩이 안된 경우
        case ELoadingState_DEPRECATED::NotLoaded:
        case ELoadingState_DEPRECATED::Failed:
        {
            // event를 새로 생성
            slot.state = ELoadingState_DEPRECATED::Loading;
            slot.load_event = std::make_shared<EventWaitHandle>();
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
    Optional<const AssetEntry_DEPRECATED&> entry_opt = registry.GetEntry(in_guid);
    SE_ASSERT(entry_opt, "Asset not found in registry: {}", in_guid.ToString());

    // Type에 맞는 Loader 가져오기
    IAssetLoader* loader = GetLoaderFromType(entry_opt->loader_type);
    SE_ASSERT(loader, "No loader registered for asset type: {}", entry_opt->loader_type.GetName());

    // vpath로부터 실제 경로 가져오기
    Path physical_path = entry_opt->virtual_path.ToPath();
    SE_ASSERT(!physical_path.IsEmpty(), "Asset path not found: {}", entry_opt->virtual_path);

    // Asset Load 및 Slot에 저장
    std::shared_ptr<IAsset> loaded_asset = co_await loader->Load(
        std::move(physical_path),
        entry_opt->import_settings.get()
    );

    {
        std::scoped_lock lock(slots_mutex);

        AssetSlot& slot = asset_slots[in_guid];
        slot.asset = loaded_asset;
        slot.state = loaded_asset ? ELoadingState_DEPRECATED::Loaded : ELoadingState_DEPRECATED::Failed;

        // 로딩 완료
        slot.load_event->Set();
        slot.load_event.reset();
    }

    co_return std::static_pointer_cast<T>(std::move(loaded_asset));
}
}  // namespace se::asset
