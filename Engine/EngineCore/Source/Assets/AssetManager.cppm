module;
#include "tracy/Tracy.hpp"
export module SE.Assets:AssetManager;
import :AssetStorage;
import :AssetHandle;

import SE.Types;
import std;


export namespace se::assets
{
/**
 *
 */
class AssetManager
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
     * 에셋을 비동기로 가져옵니다.
     * @tparam T 가져오려는 에셋 타입
     * @param virtual_path 에셋의 가상 경로 위치, ex) Assets://Foo/Bar/MyAsset.png
     * @return 에셋의 Handle
     */
    template <typename T>
        requires loaders::AssetLoadable<T>
    Optional<AssetHandle<T>> Load(const VPath& virtual_path);

    /**
     * 에셋을 동기 형태로 가져옵니다. (에셋을 불러오는 동안 Thread가 Blocking됩니다!)
     * @tparam T 가져오려는 에셋 타입
     * @param virtual_path 에셋의 가상 경로 위치, ex) Assets://Foo/Bar/MyAsset.png
     * @return 불러온 에셋
     */
    template <typename T>
        requires loaders::AssetLoadable<T>
    std::shared_ptr<T> LoadSynchronous(const VPath& virtual_path);

    /**
     * Handle로부터 실제 에셋을 가져옵니다. 만약 에셋이 아직 로딩 중이라면, 로딩이 끝날 때까지 기다립니다.
     * @tparam T 가져오려는 에셋 타입
     * @param handle 실제 에셋을 불러올 Handle
     * @return 에셋의 참조. Handle이 유효하지 않을 경우 nullptr를 반환합니다.
     */
    template <typename T>
    std::shared_ptr<T> GetAsset(AssetHandle<T> handle);

private:
    /**
     * AssetStorage를 새로 만들거나 가져옵니다.
     * @tparam T 가져오려는 Asset의 타입
     * @return AssetStorage<T>의 참조
     */
    template <typename T>
    AssetStorage<T>& GetOrCreateStorage();

private:
    TracyLockable(std::mutex, storages_mutex);
    unordered_map<std::type_index, std::shared_ptr<IAssetStorage>> storages;
};

template <typename T>
    requires loaders::AssetLoadable<T>
Optional<AssetHandle<T>> AssetManager::Load(const VPath& virtual_path)
{
    if (Optional<std::filesystem::path> physical_path_opt = core::paths::Resolve(virtual_path))
    {
        AssetHandle<T> handle = {
            .asset_id = virtual_path.ToStringName()
        };

        GetOrCreateStorage<T>().LoadAssetAsync(handle.asset_id, physical_path_opt.Value());
        return handle;
    }

    ConsoleLog(ELogLevel::Warning, u8"Failed to resolve virtual path: {}", virtual_path.ToU8String());
    return std::nullopt;
}

template <typename T>
    requires loaders::AssetLoadable<T>
std::shared_ptr<T> AssetManager::LoadSynchronous(const VPath& virtual_path)
{
    if (Optional<AssetHandle<T>> handle_opt = Load<T>(virtual_path))
    {
        return GetAsset<T>(*handle_opt);
    }
    return nullptr;
}

template <typename T>
std::shared_ptr<T> AssetManager::GetAsset(AssetHandle<T> handle)
{
    if (!handle.IsValid())
    {
        return nullptr;
    }

    AssetStorage<T>& storage = GetOrCreateStorage<T>();
    return storage.GetAssetOrWait(handle.asset_id);
}

template <typename T>
AssetStorage<T>& AssetManager::GetOrCreateStorage()
{
    const std::type_index type_idx = std::type_index(typeid(T));

    std::scoped_lock lock(storages_mutex);
    if (!storages.contains(type_idx))
    {
        storages[type_idx] = std::make_shared<AssetStorage<T>>();
    }
    return *std::static_pointer_cast<AssetStorage<T>>(storages[type_idx]);
}
}
