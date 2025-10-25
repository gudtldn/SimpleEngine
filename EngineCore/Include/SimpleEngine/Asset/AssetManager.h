#pragma once
#include <concepts>
#include <filesystem>
#include <memory>
#include <mutex>
#include <utility>

#include "SimpleEngine/Asset/AssetStorage.h"
#include "SimpleEngine/Asset/Loaders/AssetLoader.h"
#include "SimpleEngine/Core/Concurrency/TaskScheduler.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/Awaitables.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Utility/PathResolver.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
/**
 * 동일한 에셋에 대한 중복 로딩 요청을 관리하기 위한 내부 구조체
 */
struct LoadingRequest
{
private:
    using EventHandle = core::concurrency::coroutine::EventWaitHandle;

public:
    // 여러 스레드가 하나의 로딩 완료 이벤트를 기다릴 수 있도록 shared_ptr로 관리
    std::shared_ptr<EventHandle> event = std::make_shared<EventHandle>();
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
    /**
     * 에셋을 비동기적으로 로드하고, 완료되면 메인 스레드에서 콜백을 호출합니다.
     * @tparam T 로드할 에셋 타입
     * @tparam Fn void(std::shared_ptr<T>) 형태의 콜백 함수
     * @param virtual_path 에셋의 가상 경로 (예: "Assets://Textures/T_Hero.png")
     * @param on_loaded 로딩 완료 시 호출될 콜백 함수. 로딩 실패 시 nullptr가 전달됩니다.
     */
    template <typename T, typename Fn>
        requires AssetLoadable<T>
        && std::invocable<Fn, std::shared_ptr<T>>
    void LoadAsync(VPath virtual_path, Fn&& on_loaded);

    /**
     * 에셋을 동기 형태로 가져옵니다. (에셋을 불러오는 동안 Thread가 Blocking됩니다!)
     * @tparam T 가져오려는 에셋 타입
     * @param virtual_path 에셋의 가상 경로 위치, ex) Assets://Foo/Bar/MyAsset.png
     * @return 불러온 에셋
     */
    template <typename T>
        requires AssetLoadable<T>
    std::shared_ptr<T> LoadSynchronous(VPath virtual_path);

private:
    /**
     * AssetStorage를 새로 만들거나 가져옵니다.
     * @tparam T 가져오려는 Asset의 타입
     * @return AssetStorage<T>의 참조
     */
    template <typename T>
    AssetStorage<T>& GetOrCreateStorage();

    template <typename T>
    core::concurrency::Task<std::shared_ptr<T>> LoadInternal(const VPath& virtual_path);

private:
    TracyLockable(std::mutex, storages_mutex);
    unordered_map<refl::TypeId, std::shared_ptr<IAssetStorage>> storages;

    // 현재 진행 중인 로딩 요청을 추적하는 맵
    TracyLockable(std::mutex, loading_requests_mutex);
    unordered_map<StringName, LoadingRequest> loading_requests;
};

template <typename T, typename Fn>
    requires AssetLoadable<T>
    && std::invocable<Fn, std::shared_ptr<T>>
void AssetManager::LoadAsync(VPath virtual_path, Fn&& on_loaded)
{
    using namespace core;
    using namespace core::concurrency;

    TaskScheduler::Get().Launch_WorkerThread(
        [](AssetManager* self, VPath path, Function<void(std::shared_ptr<T>)> callback) -> Task<void>
        {
            std::shared_ptr<T> asset = co_await self->LoadInternal<T>(path);

            co_await coroutine::SwitchToMainThread{};
            if (callback)
            {
                callback(std::move(asset));
            }
        }(this, std::move(virtual_path), std::forward<Fn>(on_loaded))
    );
}

template <typename T>
    requires AssetLoadable<T>
std::shared_ptr<T> AssetManager::LoadSynchronous(VPath virtual_path)
{
    using core::concurrency::TaskScheduler;
    return TaskScheduler::Get().BlockOn(LoadInternal<T>(std::move(virtual_path)));
}

template <typename T>
AssetStorage<T>& AssetManager::GetOrCreateStorage()
{
    const auto type_id = refl::TypeId::Get<T>();

    std::scoped_lock lock(storages_mutex);
    if (!storages.contains(type_id))
    {
        storages[type_id] = std::make_shared<AssetStorage<T>>();
    }
    return *std::static_pointer_cast<AssetStorage<T>>(storages[type_id]);
}

template <typename T>
core::concurrency::Task<std::shared_ptr<T>> AssetManager::LoadInternal(const VPath& virtual_path)
{
    using namespace core::concurrency;

    const StringName asset_id = virtual_path.ToStringName();
    AssetStorage<T>& storage = GetOrCreateStorage<T>();

    // 캐시에 이미 있는지 확인
    if (std::shared_ptr<T> cached_asset = storage.Find(asset_id))
    {
        co_return cached_asset;
    }

    std::shared_ptr<coroutine::EventWaitHandle> ongoing_load_event;
    bool first_loader = false;
    {
        std::unique_lock lock(loading_requests_mutex);
        if (const auto it = loading_requests.find(asset_id); it != loading_requests.end())
        {
            ongoing_load_event = it->second.event;
        }
        else
        {
            first_loader = true;
            ongoing_load_event = loading_requests[asset_id].event;
        }
    }

    if (first_loader)
    {
        using utility::PathResolver;
        const PathResolver& resolver = PathResolver::Get();

        TaskScheduler::Get().Launch_WorkerThread([](
            AssetManager* self,
            Optional<std::filesystem::path> physical_path_opt,
            StringName asset_id_copy,
            std::shared_ptr<coroutine::EventWaitHandle> event
        ) -> Task<void>
            {
                std::shared_ptr<T> loaded_asset = nullptr;
                if (physical_path_opt)
                {
                    AssetLoader<T> loader;
                    loaded_asset = co_await loader.Load(physical_path_opt.Value());
                }

                // Storage에 Asset 추가
                self->GetOrCreateStorage<T>().Add(asset_id_copy, loaded_asset);

                // 로딩 완
                event->Set();

                // loading_requests 에서 제거
                std::unique_lock req_lock(self->loading_requests_mutex);
                if (self->loading_requests.contains(asset_id_copy))
                {
                    self->loading_requests.erase(asset_id_copy);
                }
            }(this, resolver.Resolve(virtual_path), asset_id, ongoing_load_event)
        );
    }

    // 로딩이 끝날 때 까지 대기
    co_await ongoing_load_event->Wait();

    // Asset 반환
    co_return storage.Find(asset_id);
}
}
