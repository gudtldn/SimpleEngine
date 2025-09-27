module;
#include "tracy/Tracy.hpp"
export module SE.Assets:AssetStorage;
import :AssetEntry;
import :Loaders;

import SE.Core;
import SE.Types;
import std;

using namespace se::core::concurrency;
using namespace se::core::concurrency::coroutine;


namespace se::assets
{
/**
 * AssetStorage를 관리하기 위한 인터페이스
 */
class IAssetStorage
{
public:
    virtual ~IAssetStorage() = default;
    virtual void RemoveReference(const StringName& asset_id) = 0;
};

/**
 * Asset을 관리하는 클래스
 * @tparam T 관리하는 Asset의 타입
 */
template <typename T>
class AssetStorage : public IAssetStorage, public std::enable_shared_from_this<AssetStorage<T>>
{
public:
    using AssetType = T;

public:
    /** 에셋을 비동기로 로드하고 Storage에 추가합니다. */
    void LoadAssetAsync(const StringName& asset_id, const std::filesystem::path& physical_path);

    /** 로딩이 완료될 때까지 기다렸다가 에셋을 반환합니다. */
    std::shared_ptr<T> GetAssetOrWait(const StringName& asset_id);

    /** Asset의 참조 카운트를 확인 후, 참조가 1개 이하일 때 Asset을 Storage에서 제거합니다. */
    virtual void RemoveReference(const StringName& asset_id) override;

private:
    TracySharedLockable(std::shared_mutex, mutex);
    unordered_map<StringName, AssetEntry<T>> asset_entries;
};

template <typename T>
void AssetStorage<T>::LoadAssetAsync(const StringName& asset_id, const std::filesystem::path& physical_path)
{
    std::promise<std::shared_ptr<T>> promise;

    {
        std::unique_lock lock(mutex);

        // 이미 로딩중이거나, 로드 되었다면 무시
        if (auto it = asset_entries.find(asset_id); it != asset_entries.end())
        {
            const AssetEntry<T>& entry = it->second;
            if (entry.state == EAssetState::Loading || entry.state == EAssetState::Loaded)
            {
                return;
            }
        }

        AssetEntry<T>& entry = asset_entries[asset_id];
        entry.state = EAssetState::Loading;
        entry.future = promise.get_future().share();
    }

    // TaskScheduler에게 코루틴과 함께 promise의 소유권을 넘겨 실행
    auto self = this->shared_from_this();
    TaskScheduler::Get().Launch_WorkerThread([self, asset_id](
        std::filesystem::path path, std::promise<std::shared_ptr<T>> prms
    ) -> Task<void>
    {
        try
        {
            // T타입에 대한 AssetLoader가 특수화 되어있는지?
            static_assert(loaders::AssetLoadable<T>, "not specialized AssetLoader for this asset type");

            // 에셋 로딩
            loaders::AssetLoader<T> loader;
            std::shared_ptr<T> asset = co_await loader.Load(path);

            {
                // Worker Thread에서 asset_entries에 접근하니까 락 걸고 진행
                std::unique_lock task_lock(self->mutex);
                self->asset_entries[asset_id].state = asset ? EAssetState::Loaded : EAssetState::Failed;
            }
            prms.set_value(asset);
        }
        catch (...)
        {
            {
                std::unique_lock task_lock(self->mutex);
                self->asset_entries[asset_id].state = EAssetState::Failed;
            }
            prms.set_exception(std::current_exception());
        }
    }(physical_path, std::move(promise)));
}

template <typename T>
std::shared_ptr<T> AssetStorage<T>::GetAssetOrWait(const StringName& asset_id)
{
    std::shared_future<std::shared_ptr<T>> future_to_wait;

    // Read Lock을 걸고 entries를 확인
    {
        std::shared_lock lock(mutex);

        auto it = asset_entries.find(asset_id);
        if (it == asset_entries.end())
        {
            // 아직 Load가 호출된 적도 없는 에셋
            return nullptr;
        }

        const AssetEntry<T>& entry = it->second;
        future_to_wait = entry.future;
    }

    if (future_to_wait.valid())
    {
        try
        {
            return future_to_wait.get();
        }
        catch (const std::exception& err)
        {
            ConsoleLog(ELogLevel::Error, u8"Failed to get asset: {}", err.what());
            return nullptr;
        }
    }
    return nullptr;
}

template <typename T>
void AssetStorage<T>::RemoveReference(const StringName& asset_id)
{
    std::unique_lock lock(mutex);
    if (auto it = asset_entries.find(asset_id); it != asset_entries.end())
    {
        using namespace std::chrono_literals;
        AssetEntry<T>& entry = it->second;

        auto status = entry.future.wait_for(0s);
        if (status == std::future_status::ready)
        {
            // AssetStorage 외부에서 아무도 참조하고 있지 않다면
            if (entry.future.get().use_count() <= 1)
            {
                asset_entries.erase(it);
            }
        }
    }
}
}
